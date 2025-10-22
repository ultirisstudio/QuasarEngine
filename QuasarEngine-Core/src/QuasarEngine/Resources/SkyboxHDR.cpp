#include "qepch.h"
#include "SkyboxHDR.h"

#include <glad/glad.h>
#include <stb_image.h>

#include <cmath>
#include <array>

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

#include <QuasarEngine/Renderer/RendererAPI.h>
#include <QuasarEngine/Shader/Shader.h>
#include <QuasarEngine/Renderer/DrawMode.h>
#include <QuasarEngine/Renderer/Buffer.h>
#include <QuasarEngine/Resources/Mesh.h>
#include <numeric>

namespace QuasarEngine
{
	static const char* extFor(RendererAPI::API api, Shader::ShaderStageType s)
	{
		if (api == RendererAPI::API::Vulkan) {
			switch (s) {
			case Shader::ShaderStageType::Vertex:     return ".vert.spv";
			case Shader::ShaderStageType::Fragment:   return ".frag.spv";
			default: return "";
			}
		}
		else {
			switch (s) {
			case Shader::ShaderStageType::Vertex:     return ".vert.glsl";
			case Shader::ShaderStageType::Fragment:   return ".frag.glsl";
			default: return "";
			}
		}
	}

	struct alignas(16) SkyboxGlobals {
		glm::mat4 view;
		glm::mat4 projection;
	};

	struct PrefilterObj { float roughness; };

	SkyboxHDR::SkyboxHDR()
	{
		glEnable(GL_TEXTURE_CUBE_MAP_SEAMLESS);

		const auto api = RendererAPI::GetAPI();
		const std::string basePath = (api == RendererAPI::API::Vulkan)
			? "Assets/Shaders/vk/spv/"
			: "Assets/Shaders/gl/";

		constexpr Shader::ShaderStageFlags GP =
			Shader::StageToBit(Shader::ShaderStageType::Vertex) |
			Shader::StageToBit(Shader::ShaderStageType::Fragment);

		// -- equirectangular_to_cubemap --
		{
			Shader::ShaderDescription d{};
			const std::string v = basePath + "cubemap" + extFor(api, Shader::ShaderStageType::Vertex);
			const std::string f = basePath + "equirectangular_to_cubemap" + extFor(api, Shader::ShaderStageType::Fragment);
			d.modules = {
				{ Shader::ShaderStageType::Vertex,   v, { {0, Shader::ShaderIOType::Vec3, "inPosition", true, ""} } },
				{ Shader::ShaderStageType::Fragment, f, {} }
			};
			d.globalUniforms = {
				{"view",       Shader::ShaderUniformType::Mat4, sizeof(glm::mat4), offsetof(SkyboxGlobals, view),       0, 0, GP},
				{"projection", Shader::ShaderUniformType::Mat4, sizeof(glm::mat4), offsetof(SkyboxGlobals, projection), 0, 0, GP},
			};
			d.blendMode = Shader::BlendMode::None;
			d.cullMode = Shader::CullMode::None;
			d.fillMode = Shader::FillMode::Solid;
			d.depthFunc = Shader::DepthFunc::Always;
			d.depthTestEnable = false; d.depthWriteEnable = false;
			d.topology = Shader::PrimitiveTopology::TriangleList;
			d.enableDynamicViewport = true; d.enableDynamicScissor = true;

			m_EquirectangularToCubemapShader = Shader::Create(d);
		}

		// -- irradiance_convolution --
		{
			Shader::ShaderDescription d{};
			const std::string v = basePath + "cubemap" + extFor(api, Shader::ShaderStageType::Vertex);
			const std::string f = basePath + "irradiance_convolution" + extFor(api, Shader::ShaderStageType::Fragment);
			d.modules = {
				{ Shader::ShaderStageType::Vertex,   v, { {0, Shader::ShaderIOType::Vec3, "inPosition", true, ""} } },
				{ Shader::ShaderStageType::Fragment, f, {} }
			};
			d.globalUniforms = {
				{"view",       Shader::ShaderUniformType::Mat4, sizeof(glm::mat4), offsetof(SkyboxGlobals, view),       0, 0, GP},
				{"projection", Shader::ShaderUniformType::Mat4, sizeof(glm::mat4), offsetof(SkyboxGlobals, projection), 0, 0, GP},
			};
			d.blendMode = Shader::BlendMode::None;
			d.cullMode = Shader::CullMode::None;
			d.fillMode = Shader::FillMode::Solid;
			d.depthFunc = Shader::DepthFunc::Always;
			d.depthTestEnable = false; d.depthWriteEnable = false;
			d.topology = Shader::PrimitiveTopology::TriangleList;
			d.enableDynamicViewport = true; d.enableDynamicScissor = true;

			m_IrradianceShader = Shader::Create(d);
		}

		// -- prefilter (uniform objet = roughness) --
		{
			constexpr Shader::ShaderStageFlags PF = GP;

			Shader::ShaderDescription d{};
			const std::string v = basePath + "cubemap" + extFor(api, Shader::ShaderStageType::Vertex);
			const std::string f = basePath + "prefilter" + extFor(api, Shader::ShaderStageType::Fragment);
			d.modules = {
				{ Shader::ShaderStageType::Vertex,   v, { {0, Shader::ShaderIOType::Vec3, "inPosition", true, ""} } },
				{ Shader::ShaderStageType::Fragment, f, {} }
			};
			d.globalUniforms = {
				{"view",       Shader::ShaderUniformType::Mat4, sizeof(glm::mat4), offsetof(SkyboxGlobals, view),       0, 0, PF},
				{"projection", Shader::ShaderUniformType::Mat4, sizeof(glm::mat4), offsetof(SkyboxGlobals, projection), 0, 0, PF},
			};
			d.objectUniforms = {
				{"roughness",  Shader::ShaderUniformType::Float, sizeof(float), offsetof(PrefilterObj, roughness), 1, 0, PF},
			};
			d.blendMode = Shader::BlendMode::None;
			d.cullMode = Shader::CullMode::None;
			d.fillMode = Shader::FillMode::Solid;
			d.depthFunc = Shader::DepthFunc::Always;
			d.depthTestEnable = false; d.depthWriteEnable = false;
			d.topology = Shader::PrimitiveTopology::TriangleList;
			d.enableDynamicViewport = true; d.enableDynamicScissor = true;

			m_PrefilterShader = Shader::Create(d);
		}

		// -- background (dessin de la skybox) --
		{
			Shader::ShaderDescription d{};
			const std::string v = basePath + "background" + extFor(api, Shader::ShaderStageType::Vertex);
			const std::string f = basePath + "background" + extFor(api, Shader::ShaderStageType::Fragment);
			d.modules = {
				{ Shader::ShaderStageType::Vertex,   v, { {0, Shader::ShaderIOType::Vec3, "inPosition", true, ""} } },
				{ Shader::ShaderStageType::Fragment, f, {} }
			};
			d.globalUniforms = {
				{"view",       Shader::ShaderUniformType::Mat4, sizeof(glm::mat4), offsetof(SkyboxGlobals, view),       0, 0, GP},
				{"projection", Shader::ShaderUniformType::Mat4, sizeof(glm::mat4), offsetof(SkyboxGlobals, projection), 0, 0, GP},
			};

			d.blendMode = Shader::BlendMode::None;
			d.fillMode = Shader::FillMode::Solid;
			d.cullMode = Shader::CullMode::Front;
			d.depthFunc = Shader::DepthFunc::LessOrEqual;
			d.depthTestEnable = true;
			d.depthWriteEnable = false;

			d.topology = Shader::PrimitiveTopology::TriangleList;
			d.enableDynamicViewport = true; d.enableDynamicScissor = true;

			m_BackgroundShader = Shader::Create(d);
		}

		// -- brdf LUT (quad en triangle strip pos+uv) --
		{
			Shader::ShaderDescription d{};
			const std::string v = basePath + "brdf" + extFor(api, Shader::ShaderStageType::Vertex);
			const std::string f = basePath + "brdf" + extFor(api, Shader::ShaderStageType::Fragment);
			d.modules = {
				{ Shader::ShaderStageType::Vertex,   v, {
					{0, Shader::ShaderIOType::Vec3, "inPosition", true, ""},
					{1, Shader::ShaderIOType::Vec2, "inTexCoord", true, ""} } },
				{ Shader::ShaderStageType::Fragment, f, {} }
			};
			
			d.blendMode = Shader::BlendMode::None;
			d.cullMode = Shader::CullMode::None;
			d.fillMode = Shader::FillMode::Solid;
			d.depthFunc = Shader::DepthFunc::Always;
			d.depthTestEnable = false; d.depthWriteEnable = false;
			d.topology = Shader::PrimitiveTopology::TriangleStrip;
			d.enableDynamicViewport = true; d.enableDynamicScissor = true;

			m_BrdfShader = Shader::Create(d);
		}

		BufferLayout cubeLayout = { { ShaderDataType::Vec3, "inPosition" } };

		std::vector<float> skyboxVertices = {
			// -Z
			-1,  1, -1,  -1, -1, -1,   1, -1, -1,
			1, -1, -1,   1,  1, -1,  -1,  1, -1,
			// -X
			-1, -1,  1,  -1, -1, -1,  -1,  1, -1,
			-1,  1, -1,  -1,  1,  1,  -1, -1,  1,
			// +X
			1, -1, -1,   1, -1,  1,   1,  1,  1,
			1,  1,  1,   1,  1, -1,   1, -1, -1,
			// +Z
			-1, -1,  1,  -1,  1,  1,   1,  1,  1,
			1,  1,  1,   1, -1,  1,  -1, -1,  1,
			// +Y
			-1,  1, -1,   1,  1, -1,   1,  1,  1,
			1,  1,  1,  -1,  1,  1,  -1,  1, -1,
			// -Y
			-1, -1, -1,  -1, -1,  1,   1, -1, -1,
			1, -1, -1,  -1, -1,  1,   1, -1,  1
		};
		std::vector<unsigned int> skyboxIdx(36);
		std::iota(skyboxIdx.begin(), skyboxIdx.end(), 0);

		m_CubeMesh = std::make_shared<Mesh>(skyboxVertices, skyboxIdx, cubeLayout, DrawMode::TRIANGLES, std::nullopt);

		BufferLayout quadLayout = {
			{ ShaderDataType::Vec3, "inPosition" },
			{ ShaderDataType::Vec2, "inTexCoord" }
		};
		std::vector<float> quadVertices = {
			//  x,   y, z,   u, v
			-1.f,  1.f, 0.f, 0.f, 1.f,
			-1.f, -1.f, 0.f, 0.f, 0.f,
			 1.f,  1.f, 0.f, 1.f, 1.f,
			 1.f, -1.f, 0.f, 1.f, 0.f,
		};
		
		std::vector<unsigned int> quadIdx = { 0,1,2,3 };
		m_QuadMesh = std::make_shared<Mesh>(quadVertices, quadIdx, quadLayout, DrawMode::TRIANGLE_STRIP, std::nullopt);

		// ---- Pipeline PBR offline (génération des maps) ----

		// Résolutions
		const unsigned int res = 1024;            // envCubemap + BRDF LUT
		const unsigned int irradianceRes = 32;    // irradiance
		const unsigned int prefilterRes = 1024; // prefilter

		// FBO/RBO
		unsigned int captureFBO = 0, captureRBO = 0;
		glGenFramebuffers(1, &captureFBO);
		glGenRenderbuffers(1, &captureRBO);
		glBindFramebuffer(GL_FRAMEBUFFER, captureFBO);
		glBindRenderbuffer(GL_RENDERBUFFER, captureRBO);
		glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT24, res, res);
		glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, captureRBO);

		// Charger HDR equirect
		stbi_set_flip_vertically_on_load(true);
		int width = 0, height = 0, nrComp = 0;
		float* data = stbi_loadf("Assets/HDR/kloofendal_48d_partly_cloudy_puresky_4k.hdr", &width, &height, &nrComp, 0);
		if (data) {
			glGenTextures(1, &hdrTexture);
			glBindTexture(GL_TEXTURE_2D, hdrTexture);
			glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB16F, width, height, 0, GL_RGB, GL_FLOAT, data);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
			stbi_image_free(data);
		}
		else {
			std::cout << "[SkyboxHDR] Failed to load HDR image.\n";
		}

		// env cubemap
		glGenTextures(1, &envCubemap);
		glBindTexture(GL_TEXTURE_CUBE_MAP, envCubemap);
		for (unsigned int i = 0; i < 6; ++i) {
			glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X + i, 0, GL_RGB16F, res, res, 0, GL_RGB, GL_FLOAT, nullptr);
		}
		glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
		glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
		glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);
		glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
		glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

		// Matrices de capture
		glm::mat4 captureProjection = glm::perspective(glm::radians(90.0f), 1.0f, 0.1f, 10.0f);
		glm::mat4 captureViews[6] =
		{
			glm::lookAt(glm::vec3(0.0f), glm::vec3(1.0f, 0.0f, 0.0f), glm::vec3(0.0f,-1.0f, 0.0f)),
			glm::lookAt(glm::vec3(0.0f), glm::vec3(-1.0f, 0.0f, 0.0f), glm::vec3(0.0f,-1.0f, 0.0f)),
			glm::lookAt(glm::vec3(0.0f), glm::vec3(0.0f, 1.0f, 0.0f), glm::vec3(0.0f, 0.0f, 1.0f)),
			glm::lookAt(glm::vec3(0.0f), glm::vec3(0.0f,-1.0f, 0.0f), glm::vec3(0.0f, 0.0f,-1.0f)),
			glm::lookAt(glm::vec3(0.0f), glm::vec3(0.0f, 0.0f, 1.0f), glm::vec3(0.0f,-1.0f, 0.0f)),
			glm::lookAt(glm::vec3(0.0f), glm::vec3(0.0f, 0.0f,-1.0f), glm::vec3(0.0f,-1.0f, 0.0f))
		};

		// Equirect -> Cubemap
		m_EquirectangularToCubemapShader->Use();
		m_EquirectangularToCubemapShader->SetUniform("projection", &captureProjection, sizeof(glm::mat4));
		glActiveTexture(GL_TEXTURE0);
		glBindTexture(GL_TEXTURE_2D, hdrTexture);

		glViewport(0, 0, res, res);
		glBindFramebuffer(GL_FRAMEBUFFER, captureFBO);
		for (unsigned int i = 0; i < 6; ++i) {
			m_EquirectangularToCubemapShader->SetUniform("view", &captureViews[i], sizeof(glm::mat4));
			m_EquirectangularToCubemapShader->UpdateGlobalState();
			m_EquirectangularToCubemapShader->UpdateObject(nullptr);

			glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0,
				GL_TEXTURE_CUBE_MAP_POSITIVE_X + i, envCubemap, 0);
			glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

			m_CubeMesh->draw();
		}
		glBindFramebuffer(GL_FRAMEBUFFER, 0);
		m_EquirectangularToCubemapShader->Unuse();

		// Mipmaps du cubemap
		glBindTexture(GL_TEXTURE_CUBE_MAP, envCubemap);
		glGenerateMipmap(GL_TEXTURE_CUBE_MAP);

		// Irradiance map
		glGenTextures(1, &irradianceMap);
		glBindTexture(GL_TEXTURE_CUBE_MAP, irradianceMap);
		for (unsigned int i = 0; i < 6; ++i) {
			glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X + i, 0, GL_RGB16F, irradianceRes, irradianceRes, 0, GL_RGB, GL_FLOAT, nullptr);
		}
		glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
		glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
		glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);
		glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
		glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

		glBindFramebuffer(GL_FRAMEBUFFER, captureFBO);
		glBindRenderbuffer(GL_RENDERBUFFER, captureRBO);
		glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT24, irradianceRes, irradianceRes);

		m_IrradianceShader->Use();
		m_IrradianceShader->SetUniform("projection", &captureProjection, sizeof(glm::mat4));
		glActiveTexture(GL_TEXTURE0);
		glBindTexture(GL_TEXTURE_CUBE_MAP, envCubemap);

		glViewport(0, 0, irradianceRes, irradianceRes);
		glBindFramebuffer(GL_FRAMEBUFFER, captureFBO);
		for (unsigned int i = 0; i < 6; ++i) {
			m_IrradianceShader->SetUniform("view", &captureViews[i], sizeof(glm::mat4));
			m_IrradianceShader->UpdateGlobalState();
			m_IrradianceShader->UpdateObject(nullptr);

			glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0,
				GL_TEXTURE_CUBE_MAP_POSITIVE_X + i, irradianceMap, 0);
			glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

			m_CubeMesh->draw();
		}
		glBindFramebuffer(GL_FRAMEBUFFER, 0);
		m_IrradianceShader->Unuse();

		// Prefilter map (mip-chain)
		glGenTextures(1, &prefilterMap);
		glBindTexture(GL_TEXTURE_CUBE_MAP, prefilterMap);
		for (unsigned int i = 0; i < 6; ++i) {
			glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X + i, 0, GL_RGB16F, prefilterRes, prefilterRes, 0, GL_RGB, GL_FLOAT, nullptr);
		}
		glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
		glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
		glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);
		glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
		glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
		glGenerateMipmap(GL_TEXTURE_CUBE_MAP);

		m_PrefilterShader->Use();
		m_PrefilterShader->SetUniform("projection", &captureProjection, sizeof(glm::mat4));
		glActiveTexture(GL_TEXTURE0);
		glBindTexture(GL_TEXTURE_CUBE_MAP, envCubemap);

		glBindFramebuffer(GL_FRAMEBUFFER, captureFBO);
		const unsigned int maxMipLevels = 5;
		for (unsigned int mip = 0; mip < maxMipLevels; ++mip) {
			unsigned int mipW = static_cast<unsigned int>(prefilterRes * std::pow(0.5, mip));
			unsigned int mipH = static_cast<unsigned int>(prefilterRes * std::pow(0.5, mip));
			glBindRenderbuffer(GL_RENDERBUFFER, captureRBO);
			glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT24, mipW, mipH);
			glViewport(0, 0, mipW, mipH);

			float roughness = (float)mip / (float)(maxMipLevels - 1);
			m_PrefilterShader->SetUniform("roughness", &roughness, sizeof(float));
			m_PrefilterShader->UpdateObject(nullptr);

			for (unsigned int i = 0; i < 6; ++i) {
				m_PrefilterShader->SetUniform("view", &captureViews[i], sizeof(glm::mat4));
				m_PrefilterShader->UpdateGlobalState();

				glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0,
					GL_TEXTURE_CUBE_MAP_POSITIVE_X + i, prefilterMap, mip);
				glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

				m_CubeMesh->draw();
			}
		}
		glBindFramebuffer(GL_FRAMEBUFFER, 0);
		m_PrefilterShader->Unuse();

		// BRDF LUT (2D)
		glGenTextures(1, &brdfLUTTexture);
		glBindTexture(GL_TEXTURE_2D, brdfLUTTexture);
		glTexImage2D(GL_TEXTURE_2D, 0, GL_RG16F, res, res, 0, GL_RG, GL_FLOAT, nullptr);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

		glBindFramebuffer(GL_FRAMEBUFFER, captureFBO);
		glBindRenderbuffer(GL_RENDERBUFFER, captureRBO);
		glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT24, res, res);
		glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, brdfLUTTexture, 0);

		glViewport(0, 0, res, res);
		m_BrdfShader->Use();
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
		m_BrdfShader->UpdateGlobalState();
		m_BrdfShader->UpdateObject(nullptr);
		m_QuadMesh->draw();

		glBindFramebuffer(GL_FRAMEBUFFER, 0);
		m_BrdfShader->Unuse();

		glDeleteRenderbuffers(1, &captureRBO);
		glDeleteFramebuffers(1, &captureFBO);
	}

	void SkyboxHDR::BindCubeMap() { glBindTexture(GL_TEXTURE_CUBE_MAP, envCubemap); }
	void SkyboxHDR::BindIrradianceMap() { glBindTexture(GL_TEXTURE_CUBE_MAP, irradianceMap); }
	void SkyboxHDR::BindPrefilterMap() { glBindTexture(GL_TEXTURE_CUBE_MAP, prefilterMap); }
	void SkyboxHDR::BindBrdfLUT() { glBindTexture(GL_TEXTURE_2D, brdfLUTTexture); }

	void SkyboxHDR::Draw(const glm::mat4& view, const glm::mat4& projection)
	{
		glm::mat4 v = view;
		glm::mat4 p = projection;

		m_BackgroundShader->Use();
		m_BackgroundShader->SetUniform("view", &v, sizeof(glm::mat4));
		m_BackgroundShader->SetUniform("projection", &p, sizeof(glm::mat4));
		m_BackgroundShader->UpdateGlobalState();
		m_BackgroundShader->UpdateObject(nullptr);

		glActiveTexture(GL_TEXTURE0);
		glBindTexture(GL_TEXTURE_CUBE_MAP, envCubemap);
		//glBindTexture(GL_TEXTURE_CUBE_MAP, irradianceMap);

		m_CubeMesh->draw();

		m_BackgroundShader->Unuse();
	}
}
