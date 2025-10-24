/*#include "qepch.h"
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
			d.cullMode = Shader::CullMode::None;
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
}*/

#include "qepch.h"
#include "SkyboxHDR.h"

#include <numeric>
#include <vector>
#include <cmath>

#include <stb_image.h>
#include <QuasarEngine/Core/Logger.h>

#include <glad/glad.h>

#include <Platform/OpenGL/OpenGLFramebuffer.h>

namespace QuasarEngine
{
    const char* SkyboxHDR::ExtFor(RendererAPI::API api, Shader::ShaderStageType s)
    {
        if (api == RendererAPI::API::Vulkan) {
            switch (s) {
            case Shader::ShaderStageType::Vertex:   return ".vert.spv";
            case Shader::ShaderStageType::Fragment: return ".frag.spv";
            default: return "";
            }
        }
        else {
            switch (s) {
            case Shader::ShaderStageType::Vertex:   return ".vert.glsl";
            case Shader::ShaderStageType::Fragment: return ".frag.glsl";
            default: return "";
            }
        }
    }

    uint32_t SkyboxHDR::CalcMipCount(uint32_t w, uint32_t h)
    {
        uint32_t levels = 1;
        while (w > 1 || h > 1) {
            w = std::max(1u, w / 2);
            h = std::max(1u, h / 2);
            ++levels;
        }
        return levels;
    }

    SkyboxHDR::SkyboxHDR(const Settings& s) : m_Settings(s)
    {
        RenderCommand::Instance().SetSeamlessCubemap(true);

        CreateMeshes();
        CreateShaders();

        m_CaptureProjection = glm::perspective(glm::radians(90.0f), 1.0f, 0.1f, 10.0f);
        m_CaptureViews[0] = glm::lookAt(glm::vec3(0.0f), glm::vec3(1.0f, 0.0f, 0.0f), glm::vec3(0.0f, -1.0f, 0.0f));
        m_CaptureViews[1] = glm::lookAt(glm::vec3(0.0f), glm::vec3(-1.0f, 0.0f, 0.0f), glm::vec3(0.0f, -1.0f, 0.0f));
        m_CaptureViews[2] = glm::lookAt(glm::vec3(0.0f), glm::vec3(0.0f, 1.0f, 0.0f), glm::vec3(0.0f, 0.0f, 1.0f));
        m_CaptureViews[3] = glm::lookAt(glm::vec3(0.0f), glm::vec3(0.0f, -1.0f, 0.0f), glm::vec3(0.0f, 0.0f, -1.0f));
        m_CaptureViews[4] = glm::lookAt(glm::vec3(0.0f), glm::vec3(0.0f, 0.0f, 1.0f), glm::vec3(0.0f, -1.0f, 0.0f));
        m_CaptureViews[5] = glm::lookAt(glm::vec3(0.0f), glm::vec3(0.0f, 0.0f, -1.0f), glm::vec3(0.0f, -1.0f, 0.0f));

        FramebufferSpecification fbSpec{};
        fbSpec.Width = m_Settings.envRes;
        fbSpec.Height = m_Settings.envRes;
        fbSpec.Attachments = { { FramebufferTextureSpecification{ FramebufferTextureFormat::DEPTH24STENCIL8 } } };
        fbSpec.Samples = 1;
        m_FBO = Framebuffer::Create(fbSpec);

        LoadHDR(m_Settings.hdrPath);
        CreateTargets();

        BuildEnvironment();
        BuildIrradiance();
        BuildPrefilter();
        BuildBrdfLUT();
    }

    void SkyboxHDR::CreateMeshes()
    {
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
    }

    void SkyboxHDR::CreateShaders()
    {
        const auto api = RendererAPI::GetAPI();
        const std::string basePath = (api == RendererAPI::API::Vulkan) ? "Assets/Shaders/vk/spv/" : "Assets/Shaders/gl/";

        constexpr Shader::ShaderStageFlags GP =
            Shader::StageToBit(Shader::ShaderStageType::Vertex) |
            Shader::StageToBit(Shader::ShaderStageType::Fragment);

        struct SkyboxGlobals { glm::mat4 view; glm::mat4 projection; };
        struct PrefilterObj { float roughness; };

        {
            Shader::ShaderDescription d{};
            const std::string v = basePath + "cubemap" + ExtFor(api, Shader::ShaderStageType::Vertex);
            const std::string f = basePath + "equirectangular_to_cubemap" + ExtFor(api, Shader::ShaderStageType::Fragment);
            d.modules = {
                { Shader::ShaderStageType::Vertex,   v, { {0, Shader::ShaderIOType::Vec3, "inPosition", true, ""} } },
                { Shader::ShaderStageType::Fragment, f, {} }
            };
            d.globalUniforms = {
                {"view",       Shader::ShaderUniformType::Mat4, sizeof(glm::mat4), offsetof(SkyboxGlobals, view),       0, 0, GP},
                {"projection", Shader::ShaderUniformType::Mat4, sizeof(glm::mat4), offsetof(SkyboxGlobals, projection), 0, 0, GP},
            };
            d.samplers = {
                { "equirectangularMap", 1, 1, GP }
            };
            d.blendMode = Shader::BlendMode::None;
            d.cullMode = Shader::CullMode::None;
            d.fillMode = Shader::FillMode::Solid;
            d.depthFunc = Shader::DepthFunc::Always;
            d.depthTestEnable = true; d.depthWriteEnable = false;
            d.topology = Shader::PrimitiveTopology::TriangleList;
            d.enableDynamicViewport = true; d.enableDynamicScissor = true;

            m_EquirectangularToCubemapShader = Shader::Create(d);
        }

        {
            Shader::ShaderDescription d{};
            const std::string v = basePath + "cubemap" + ExtFor(api, Shader::ShaderStageType::Vertex);
            const std::string f = basePath + "irradiance_convolution" + ExtFor(api, Shader::ShaderStageType::Fragment);
            d.modules = {
                { Shader::ShaderStageType::Vertex,   v, { {0, Shader::ShaderIOType::Vec3, "inPosition", true, ""} } },
                { Shader::ShaderStageType::Fragment, f, {} }
            };
            d.globalUniforms = {
                {"view",       Shader::ShaderUniformType::Mat4, sizeof(glm::mat4), offsetof(SkyboxGlobals, view),       0, 0, GP},
                {"projection", Shader::ShaderUniformType::Mat4, sizeof(glm::mat4), offsetof(SkyboxGlobals, projection), 0, 0, GP},
            };
            d.samplers = {
                { "environmentMap", 1, 0, GP }
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

        {
            Shader::ShaderDescription d{};
            const std::string v = basePath + "cubemap" + ExtFor(api, Shader::ShaderStageType::Vertex);
            const std::string f = basePath + "prefilter" + ExtFor(api, Shader::ShaderStageType::Fragment);
            d.modules = {
                { Shader::ShaderStageType::Vertex,   v, { {0, Shader::ShaderIOType::Vec3, "inPosition", true, ""} } },
                { Shader::ShaderStageType::Fragment, f, {} }
            };
            d.globalUniforms = {
                {"view",       Shader::ShaderUniformType::Mat4, sizeof(glm::mat4), offsetof(SkyboxGlobals, view),       0, 0, GP},
                {"projection", Shader::ShaderUniformType::Mat4, sizeof(glm::mat4), offsetof(SkyboxGlobals, projection), 0, 0, GP},
            };
            d.objectUniforms = {
                {"roughness",  Shader::ShaderUniformType::Float, sizeof(float), offsetof(PrefilterObj, roughness), 1, 0, GP},
            };
            d.samplers = {
                { "environmentMap", 1, 1, GP }
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

        {
            Shader::ShaderDescription d{};
            const std::string v = basePath + "background" + ExtFor(api, Shader::ShaderStageType::Vertex);
            const std::string f = basePath + "background" + ExtFor(api, Shader::ShaderStageType::Fragment);
            d.modules = {
                { Shader::ShaderStageType::Vertex,   v, { {0, Shader::ShaderIOType::Vec3, "inPosition", true, ""} } },
                { Shader::ShaderStageType::Fragment, f, {} }
            };
            d.globalUniforms = {
                {"view",       Shader::ShaderUniformType::Mat4, sizeof(glm::mat4), offsetof(SkyboxGlobals, view),       0, 0, GP},
                {"projection", Shader::ShaderUniformType::Mat4, sizeof(glm::mat4), offsetof(SkyboxGlobals, projection), 0, 0, GP},
            };
            d.samplers = {
                { "environmentMap", 1, 0, Shader::StageToBit(Shader::ShaderStageType::Fragment) }
            };
            d.blendMode = Shader::BlendMode::None;
            d.fillMode = Shader::FillMode::Solid;
            d.cullMode = Shader::CullMode::None;
            d.depthFunc = Shader::DepthFunc::LessOrEqual;
            d.depthTestEnable = true;
            d.depthWriteEnable = false;
            d.topology = Shader::PrimitiveTopology::TriangleList;
            d.enableDynamicViewport = true; d.enableDynamicScissor = true;
            m_BackgroundShader = Shader::Create(d);
        }

        {
            Shader::ShaderDescription d{};
            const std::string v = basePath + "brdf" + ExtFor(api, Shader::ShaderStageType::Vertex);
            const std::string f = basePath + "brdf" + ExtFor(api, Shader::ShaderStageType::Fragment);
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
    }

    void SkyboxHDR::LoadHDR(const std::string& hdrPath)
    {
        stbi_set_flip_vertically_on_load(true);
        int w = 0, h = 0, comp = 0;
        float* hdr = stbi_loadf(hdrPath.c_str(), &w, &h, &comp, 3);
        if (!hdr) {
            Q_WARNING("SkyboxHDR: failed to load HDR image '%s'", hdrPath.c_str());
            return;
        }

        TextureSpecification spec{};
        spec.width = static_cast<uint32_t>(w);
        spec.height = static_cast<uint32_t>(h);
        spec.channels = 3;
        spec.format = TextureFormat::RGB;
        spec.internal_format = TextureFormat::RGB16F;
        spec.wrap_s = TextureWrap::CLAMP_TO_EDGE;
        spec.wrap_t = TextureWrap::CLAMP_TO_EDGE;
        spec.min_filter_param = TextureFilter::LINEAR;
        spec.mag_filter_param = TextureFilter::LINEAR;
        spec.mipmap = false;
        spec.compressed = false;

        m_HDRTexture = Texture2D::Create(spec);

        const std::size_t bytes = static_cast<std::size_t>(w) * static_cast<std::size_t>(h) * 3u * sizeof(float);
        m_HDRTexture->LoadFromData(ByteView{ reinterpret_cast<const std::uint8_t*>(hdr), bytes });

        stbi_image_free(hdr);
    }

    void SkyboxHDR::CreateTargets()
    {
        {
            TextureSpecification spec{};
            spec.width = m_Settings.envRes;
            spec.height = m_Settings.envRes;
            spec.channels = 3;
            spec.format = TextureFormat::RGB;
            spec.internal_format = TextureFormat::RGB16F;
            spec.wrap_r = spec.wrap_s = spec.wrap_t = TextureWrap::CLAMP_TO_EDGE;
            spec.min_filter_param = TextureFilter::LINEAR; // LINEAR_MIPMAP_LINEAR
            spec.mag_filter_param = TextureFilter::LINEAR;
            spec.mipmap = false; // true
            spec.compressed = false;

            m_EnvCubemap = TextureCubeMap::Create(spec);

            const std::size_t faceFloats = static_cast<std::size_t>(spec.width) * static_cast<std::size_t>(spec.height) * 3u;
            std::vector<float> zeros(faceFloats, 1.0f);
            for (int f = 0; f < 6; ++f)
                m_EnvCubemap->LoadFaceFromData(TextureCubeMap::Face(f), ByteView{ reinterpret_cast<const std::uint8_t*>(zeros.data()), faceFloats * sizeof(float) }, spec.width, spec.height, 3);
        }

        {
            TextureSpecification spec{};
            spec.width = m_Settings.irradianceRes;
            spec.height = m_Settings.irradianceRes;
            spec.channels = 3;
            spec.format = TextureFormat::RGB;
            spec.internal_format = TextureFormat::RGB16F;
            spec.wrap_r = spec.wrap_s = spec.wrap_t = TextureWrap::CLAMP_TO_EDGE;
            spec.min_filter_param = TextureFilter::LINEAR;
            spec.mag_filter_param = TextureFilter::LINEAR;
            spec.mipmap = false;
            spec.compressed = false;

            m_IrradianceMap = TextureCubeMap::Create(spec);
            const std::size_t faceFloats = static_cast<std::size_t>(spec.width) * static_cast<std::size_t>(spec.height) * 3u;
            std::vector<float> zeros(faceFloats, 0.0f);
            for (int f = 0; f < 6; ++f)
                m_IrradianceMap->LoadFaceFromData(TextureCubeMap::Face(f), ByteView{ reinterpret_cast<const std::uint8_t*>(zeros.data()), faceFloats * sizeof(float) }, spec.width, spec.height, 3);
        }

        {
            TextureSpecification spec{};
            spec.width = m_Settings.prefilterRes;
            spec.height = m_Settings.prefilterRes;
            spec.channels = 3;
            spec.format = TextureFormat::RGB;
            spec.internal_format = TextureFormat::RGB16F;
            spec.wrap_r = spec.wrap_s = spec.wrap_t = TextureWrap::CLAMP_TO_EDGE;
            spec.min_filter_param = TextureFilter::LINEAR_MIPMAP_LINEAR;
            spec.mag_filter_param = TextureFilter::LINEAR;
            spec.mipmap = true;
            spec.compressed = false;

            m_PrefilterMap = TextureCubeMap::Create(spec);
            const std::size_t faceFloats = static_cast<std::size_t>(spec.width) * static_cast<std::size_t>(spec.height) * 3u;
            std::vector<float> zeros(faceFloats, 0.0f);
            for (int f = 0; f < 6; ++f)
                m_PrefilterMap->LoadFaceFromData(TextureCubeMap::Face(f), ByteView{ reinterpret_cast<const std::uint8_t*>(zeros.data()), faceFloats * sizeof(float) }, spec.width, spec.height, 3);
        }

        {
            TextureSpecification spec{};
            spec.width = m_Settings.brdfRes;
            spec.height = m_Settings.brdfRes;
            spec.channels = 2;
            spec.format = TextureFormat::RG16F;
            spec.internal_format = TextureFormat::RG16F;
            spec.wrap_s = spec.wrap_t = TextureWrap::CLAMP_TO_EDGE;
            spec.min_filter_param = TextureFilter::LINEAR;
            spec.mag_filter_param = TextureFilter::LINEAR;
            spec.mipmap = false;
            spec.compressed = false;

            m_BrdfLUT = Texture2D::Create(spec);
            const std::size_t texFloats = static_cast<std::size_t>(spec.width) * static_cast<std::size_t>(spec.height) * 2u;
            std::vector<float> zeros(texFloats, 0.0f);
            m_BrdfLUT->LoadFromData(ByteView{ reinterpret_cast<const std::uint8_t*>(zeros.data()), texFloats * sizeof(float) });
        }
    }

    void SkyboxHDR::BuildEnvironment()
    {
        if (!m_HDRTexture || !m_EnvCubemap || !m_FBO || !m_EquirectangularToCubemapShader) return;

        m_FBO->Bind();
        m_FBO->Resize(m_Settings.envRes, m_Settings.envRes);
        m_FBO->Bind();

        m_EquirectangularToCubemapShader->Use();
        m_EquirectangularToCubemapShader->SetUniform("projection", &m_CaptureProjection, sizeof(glm::mat4));
        m_EquirectangularToCubemapShader->SetTexture("equirectangularMap", m_HDRTexture.get(), Shader::SamplerType::Sampler2D);
        m_EquirectangularToCubemapShader->UpdateGlobalState();
        m_EquirectangularToCubemapShader->UpdateObject(nullptr);

        m_HDRTexture->Bind();

        for (uint32_t face = 0; face < 6; ++face)
        {
            m_FBO->Bind();

            m_EquirectangularToCubemapShader->SetUniform("view", &m_CaptureViews[face], sizeof(glm::mat4));
            m_EquirectangularToCubemapShader->UpdateGlobalState();

            m_FBO->Bind();
            m_FBO->SetColorAttachment(0, AttachmentRef{ m_EnvCubemap, 0, face });
            m_FBO->Bind();

            m_FBO->Bind();
            m_FBO->ClearColor(0.f, 0.f, 0.f, 1.f);
            m_FBO->Bind();
            m_FBO->Clear(ClearFlags::Color | ClearFlags::Depth);
            m_FBO->Bind();

            m_CubeMesh->draw();

            m_FBO->Bind();
            m_FBO->Unbind();
        }

        m_EquirectangularToCubemapShader->Unuse();
    }

    /*void SkyboxHDR::BuildEnvironment()
    {
        if (!m_HDRTexture || !m_EnvCubemap) return;

        const GLuint cubeTex = static_cast<GLuint>(m_EnvCubemap->GetHandle());
        const GLuint hdrTex = static_cast<GLuint>(m_HDRTexture->GetHandle());

        glTextureParameteri(cubeTex, GL_TEXTURE_MAX_LEVEL, 0);
        glTextureParameteri(cubeTex, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTextureParameteri(cubeTex, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

        GLuint fbo = 0, rbo = 0;
        glCreateFramebuffers(1, &fbo);
        glCreateRenderbuffers(1, &rbo);
        glNamedRenderbufferStorage(rbo, GL_DEPTH_COMPONENT24, (GLint)m_Settings.envRes, (GLint)m_Settings.envRes);
        glNamedFramebufferRenderbuffer(fbo, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, rbo);

        const GLenum drawBuf = GL_COLOR_ATTACHMENT0;
        glNamedFramebufferDrawBuffers(fbo, 1, &drawBuf);

        glEnable(GL_TEXTURE_CUBE_MAP_SEAMLESS);
        glViewport(0, 0, (GLint)m_Settings.envRes, (GLint)m_Settings.envRes);
        glDisable(GL_CULL_FACE);
        glEnable(GL_DEPTH_TEST);

        m_EquirectangularToCubemapShader->Use();
        m_EquirectangularToCubemapShader->SetUniform("projection", &m_CaptureProjection, sizeof(glm::mat4));
        m_EquirectangularToCubemapShader->SetTexture("equirectangularMap", m_HDRTexture.get(), Shader::SamplerType::Sampler2D);
        m_EquirectangularToCubemapShader->UpdateGlobalState();
        m_EquirectangularToCubemapShader->UpdateObject(nullptr);

        glBindTextureUnit(0, hdrTex);

        for (int face = 0; face < 6; ++face)
        {
            m_EquirectangularToCubemapShader->SetUniform("view", &m_CaptureViews[face], sizeof(glm::mat4));
            m_EquirectangularToCubemapShader->UpdateGlobalState();

            glNamedFramebufferTextureLayer(fbo, GL_COLOR_ATTACHMENT0, cubeTex, 0, face);

            const GLenum status = glCheckNamedFramebufferStatus(fbo, GL_FRAMEBUFFER);
            if (status != GL_FRAMEBUFFER_COMPLETE) {
                Q_WARNING("BuildEnvironment: FBO incomplete for face %d (status=0x%X)", face, status);
                continue;
            }

            glBindFramebuffer(GL_FRAMEBUFFER, fbo);
            glDrawBuffer(GL_COLOR_ATTACHMENT0);
            glClearColor(0.f, 0.f, 0.f, 1.f);
            glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

            m_CubeMesh->draw();

            glBindFramebuffer(GL_FRAMEBUFFER, 0);
        }

        m_EquirectangularToCubemapShader->Unuse();

        glDeleteRenderbuffers(1, &rbo);
        glDeleteFramebuffers(1, &fbo);
    }*/

    void SkyboxHDR::BuildIrradiance()
    {
        if (!m_IrradianceMap || !m_EnvCubemap) return;

        m_FBO->Resize(m_Settings.irradianceRes, m_Settings.irradianceRes);

        m_IrradianceShader->Use();

        for (uint32_t face = 0; face < 6; ++face) {
            m_IrradianceShader->SetUniform("view", &m_CaptureViews[face], sizeof(glm::mat4));
            m_IrradianceShader->SetUniform("projection", &m_CaptureProjection, sizeof(glm::mat4));
            m_IrradianceShader->SetTexture("environmentMap", m_EnvCubemap.get(), Shader::SamplerType::SamplerCube);
            m_IrradianceShader->UpdateGlobalState();
            m_IrradianceShader->UpdateObject(nullptr);

            m_FBO->Bind();
            m_FBO->SetColorAttachment(0, AttachmentRef{ m_IrradianceMap, 0, face });
            m_FBO->Clear(ClearFlags::All);

            m_FBO->Bind();
            m_CubeMesh->draw();
            m_FBO->Unbind();
        }

        m_IrradianceShader->Unuse();
    }

    void SkyboxHDR::BuildPrefilter()
    {
        if (!m_PrefilterMap || !m_EnvCubemap) return;

        const uint32_t mipMax = std::min<uint32_t>(5, CalcMipCount(m_Settings.prefilterRes, m_Settings.prefilterRes)); // souvent 5
        m_PrefilterShader->Use();

        for (uint32_t mip = 0; mip < mipMax; ++mip) {
            const uint32_t w = std::max(1u, m_Settings.prefilterRes >> mip);
            const uint32_t h = std::max(1u, m_Settings.prefilterRes >> mip);

            m_FBO->Resize(w, h);

            float roughness = (mipMax <= 1) ? 0.0f : (float)mip / float(mipMax - 1);

            for (uint32_t face = 0; face < 6; ++face) {
                m_PrefilterShader->SetUniform("view", &m_CaptureViews[face], sizeof(glm::mat4));
                m_PrefilterShader->SetUniform("projection", &m_CaptureProjection, sizeof(glm::mat4));
                m_PrefilterShader->SetTexture("environmentMap", m_EnvCubemap.get(), Shader::SamplerType::SamplerCube);
                m_PrefilterShader->SetUniform("roughness", &roughness, sizeof(float));
                m_PrefilterShader->UpdateGlobalState();
                m_PrefilterShader->UpdateObject(nullptr);

                m_FBO->Bind();
                m_FBO->SetColorAttachment(0, AttachmentRef{ m_PrefilterMap, mip, face });
                m_FBO->Clear(ClearFlags::All);

                m_CubeMesh->draw();
                m_FBO->Unbind();
            }
        }

        m_PrefilterShader->Unuse();
    }

    void SkyboxHDR::BuildBrdfLUT()
    {
        if (!m_BrdfLUT) return;

        m_FBO->Resize(m_Settings.brdfRes, m_Settings.brdfRes);

        m_BrdfShader->Use();
        m_BrdfShader->UpdateGlobalState();
        m_BrdfShader->UpdateObject(nullptr);

        m_FBO->Bind();
        m_FBO->SetColorAttachment(0, AttachmentRef{ m_BrdfLUT, 0, 0 });
        m_FBO->Clear(ClearFlags::Color);

        m_QuadMesh->draw();

        m_FBO->Unbind();
        m_BrdfShader->Unuse();
    }

    void SkyboxHDR::Draw(const glm::mat4& view, const glm::mat4& projection)
    {
        if (!m_BackgroundShader || !m_EnvCubemap) return;

        glm::mat4 v = view;
        glm::mat4 p = projection;

        m_BackgroundShader->Use();
        m_BackgroundShader->SetUniform("view", &v, sizeof(glm::mat4));
        m_BackgroundShader->SetUniform("projection", &p, sizeof(glm::mat4));
        m_BackgroundShader->SetTexture("environmentMap", m_EnvCubemap.get(), Shader::SamplerType::SamplerCube);
        //m_BackgroundShader->SetTexture("environmentMap", m_IrradianceMap.get(), Shader::SamplerType::SamplerCube);

        m_BackgroundShader->UpdateGlobalState();
        m_BackgroundShader->UpdateObject(nullptr);

        m_CubeMesh->draw();

        m_BackgroundShader->Unuse();
    }
}
