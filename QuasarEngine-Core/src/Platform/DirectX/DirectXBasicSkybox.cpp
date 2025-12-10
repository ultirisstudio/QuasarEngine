#include "qepch.h"

#include "DirectXBasicSkybox.h"

#include <stb_image.h>

#include <numeric>

#include <QuasarEngine/Shader/Shader.h>

namespace QuasarEngine
{
	DirectXBasicSkybox::DirectXBasicSkybox()
	{
		Shader::ShaderDescription desc;

		desc.modules = {
			Shader::ShaderModuleInfo{
				Shader::ShaderStageType::Vertex,
				"Assets/Shaders/basic_skybox.vert.gl.glsl",
				"",
				{
					{0, Shader::ShaderIOType::Vec3, "inPosition", true, ""},
					{1, Shader::ShaderIOType::Vec3, "inNormal",   true, ""},
					{2, Shader::ShaderIOType::Vec2, "inTexCoord", true, ""}
				}
			},
			Shader::ShaderModuleInfo{
				Shader::ShaderStageType::Fragment,
				"Assets/Shaders/basic_skybox.frag.gl.glsl",
				"",
				{
					{0, Shader::ShaderIOType::Vec3, "inTexCoord", true, ""}
				}
			}
		};

		desc.globalUniforms = {
			{"view",  Shader::ShaderUniformType::Mat4, sizeof(glm::mat4), 0, 0, 0,Shader::StageToBit(Shader::ShaderStageType::Vertex)},
			{"projection",  Shader::ShaderUniformType::Mat4, sizeof(glm::mat4),   sizeof(glm::mat4), 0, 0, Shader::StageToBit(Shader::ShaderStageType::Vertex)}
		};

		desc.objectUniforms = {
			{"model", Shader::ShaderUniformType::Mat4, sizeof(glm::mat4), 0, 1, 0, Shader::StageToBit(Shader::ShaderStageType::Fragment)}
		};

		desc.samplers = {
			{"skybox", 1, 1, Shader::StageToBit(Shader::ShaderStageType::Fragment)}
		};

		desc.blendMode = Shader::BlendMode::None;
		desc.cullMode = Shader::CullMode::None;
		desc.fillMode = Shader::FillMode::Solid;
		desc.depthTestEnable = true;
		desc.depthWriteEnable = false;
		desc.depthFunc = Shader::DepthFunc::LessOrEqual;
		desc.topology = Shader::PrimitiveTopology::TriangleList;
		desc.enableDynamicViewport = true;
		desc.enableDynamicScissor = true;
		desc.enableDynamicLineWidth = false;

		shader = Shader::Create(desc);

		MaterialSpecification materialSpec;
		material = Material::CreateMaterial(materialSpec);

		shader->AcquireResources(material.get());

		material->m_Generation++;

		TextureSpecification textureSpec;
		textureSpec.width = 2048;
		textureSpec.height = 2048;
		texture = std::make_shared<DirectXTexture2D>(textureSpec);

		std::vector<float> cubeVertices = {
			-1,  1, -1,   0,  0, -1,   0, 1,
			-1, -1, -1,   0,  0, -1,   0, 0,
			 1, -1, -1,   0,  0, -1,   1, 0,
			 1, -1, -1,   0,  0, -1,   1, 0,
			 1,  1, -1,   0,  0, -1,   1, 1,
			-1,  1, -1,   0,  0, -1,   0, 1,

			-1, -1,  1,   0,  0,  1,   0, 0,
			-1,  1,  1,   0,  0,  1,   0, 1,
			 1,  1,  1,   0,  0,  1,   1, 1,
			 1,  1,  1,   0,  0,  1,   1, 1,
			 1, -1,  1,   0,  0,  1,   1, 0,
			-1, -1,  1,   0,  0,  1,   0, 0,

			-1,  1,  1,  -1,  0,  0,   1, 1,
			-1,  1, -1,  -1,  0,  0,   0, 1,
			-1, -1, -1,  -1,  0,  0,   0, 0,
			-1, -1, -1,  -1,  0,  0,   0, 0,
			-1, -1,  1,  -1,  0,  0,   1, 0,
			-1,  1,  1,  -1,  0,  0,   1, 1,

			 1,  1,  1,   1,  0,  0,   1, 1,
			 1, -1,  1,   1,  0,  0,   1, 0,
			 1, -1, -1,   1,  0,  0,   0, 0,
			 1, -1, -1,   1,  0,  0,   0, 0,
			 1,  1, -1,   1,  0,  0,   0, 1,
			 1,  1,  1,   1,  0,  0,   1, 1,

			-1, -1, -1,   0, -1,  0,   0, 1,
			 1, -1, -1,   0, -1,  0,   1, 1,
			 1, -1,  1,   0, -1,  0,   1, 0,
			 1, -1,  1,   0, -1,  0,   1, 0,
			-1, -1,  1,   0, -1,  0,   0, 0,
			-1, -1, -1,   0, -1,  0,   0, 1,

			-1,  1, -1,   0,  1,  0,   0, 1,
			-1,  1,  1,   0,  1,  0,   0, 0,
			 1,  1,  1,   0,  1,  0,   1, 0,
			 1,  1,  1,   0,  1,  0,   1, 0,
			 1,  1, -1,   0,  1,  0,   1, 1,
			-1,  1, -1,   0,  1,  0,   0, 1
		};

		std::vector<uint32_t> cubeIndices(36);
		std::iota(cubeIndices.begin(), cubeIndices.end(), 0);

		cubeMesh = std::make_unique<Mesh>(cubeVertices, cubeIndices, std::nullopt, DrawMode::TRIANGLES);
	}

	DirectXBasicSkybox::~DirectXBasicSkybox()
	{
		shader->ReleaseResources(material.get());

		texture.reset();
		material.reset();
		cubeMesh.reset();
		shader.reset();
	}

	void DirectXBasicSkybox::Bind()
	{
		shader->Use();
	}

	void DirectXBasicSkybox::Unbind()
	{
		shader->Unuse();
	}

	void DirectXBasicSkybox::Draw()
	{
		cubeMesh->draw();
	}

	void DirectXBasicSkybox::LoadCubemap(const std::array<std::string, 6>& faces)
	{
		int width, height, channels;
		const int desiredChannels = 4;
		bool success = true;

		for (unsigned int i = 0; i < faces.size(); i++)
		{
			unsigned char* data = stbi_load(faces[i].c_str(), &width, &height, &channels, desiredChannels);
			if (data)
			{
				stbi_image_free(data);
			}
			else
			{
				std::cerr << "Failed to load cubemap face [" << i << "]: " << faces[i] << "\nReason: " << stbi_failure_reason() << std::endl;
				success = false;
				break;
			}
		}

		if (!success)
		{
			//texture->m_ID = 0;
			throw std::runtime_error("Failed to load one or more faces of the cubemap");
		}

		material->SetTexture(TextureType::Albedo, texture.get());
	}

	Shader* DirectXBasicSkybox::GetShader()
	{
		return shader.get();
	}

	Texture2D* DirectXBasicSkybox::GetTexture()
	{
		return texture.get();
	}

	Material* DirectXBasicSkybox::GetMaterial()
	{
		return material.get();
	}
}
