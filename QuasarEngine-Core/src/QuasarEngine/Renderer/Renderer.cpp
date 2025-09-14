#include "qepch.h"

#include <GLFW/glfw3.h>

#include <glm/gtc/type_ptr.hpp>
#include <glm/gtx/matrix_decompose.hpp>

#include <QuasarEngine/Renderer/Renderer.h>

#include <QuasarEngine/Entity/Entity.h>
#include <QuasarEngine/Resources/Model.h>
#include <QuasarEngine/Resources/Materials/Material.h>
#include <QuasarEngine/Resources/Mesh.h>

#include <QuasarEngine/Entity/AllComponents.h>

#include <QuasarEngine/Tools/Math.h>

#include "QuasarEngine/Core/Logger.h"

#include <glad/glad.h>

namespace QuasarEngine
{
	Renderer::SceneData Renderer::m_SceneData = Renderer::SceneData();

	void Renderer::Init()
	{
		RenderCommand::Init();

		Shader::ShaderDescription desc;

		std::string vertPath = "Assets/Shaders/basic.vert." + std::string(RendererAPI::GetAPI() == RendererAPI::API::Vulkan ? "spv" : "gl.glsl");
		std::string fragPath = "Assets/Shaders/basic.frag." + std::string(RendererAPI::GetAPI() == RendererAPI::API::Vulkan ? "spv" : "gl.glsl");

		desc.modules = {
			Shader::ShaderModuleInfo{
				Shader::ShaderStageType::Vertex,
				vertPath,
				{
					{0, Shader::ShaderIOType::Vec3, "inPosition", true, ""},
					{1, Shader::ShaderIOType::Vec3, "inNormal",   true, ""},
					{2, Shader::ShaderIOType::Vec2, "inTexCoord", true, ""}
				}
			},
			Shader::ShaderModuleInfo{
				Shader::ShaderStageType::Fragment,
				fragPath,
				{}
			}
		};

		struct alignas(16) GlobalUniforms
		{
			glm::mat4 view;
			glm::mat4 projection;
			glm::vec3 camera_position;
			int usePointLight;
			int useDirLight;
			PointLight pointLights[4];
			DirectionalLight dirLights[4];
		};
		static_assert(offsetof(GlobalUniforms, pointLights) % 16 == 0, "pointLights offset must be 16-aligned");
		static_assert(offsetof(GlobalUniforms, dirLights) % 16 == 0, "dirLights offset must be 16-aligned");

		Shader::ShaderStageFlags globalUniformsFlags = Shader::StageToBit(Shader::ShaderStageType::Vertex) | Shader::StageToBit(Shader::ShaderStageType::Fragment);

		desc.globalUniforms = {
			{"view", Shader::ShaderUniformType::Mat4, sizeof(glm::mat4), offsetof(GlobalUniforms, view), 0, 0, globalUniformsFlags},
			{"projection", Shader::ShaderUniformType::Mat4, sizeof(glm::mat4), offsetof(GlobalUniforms, projection), 0, 0, globalUniformsFlags},
			{"camera_position", Shader::ShaderUniformType::Vec3, sizeof(glm::vec3), offsetof(GlobalUniforms, camera_position), 0, 0, globalUniformsFlags},
			{"usePointLight", Shader::ShaderUniformType::Int, sizeof(int), offsetof(GlobalUniforms, usePointLight), 0, 0, globalUniformsFlags},
			{"useDirLight", Shader::ShaderUniformType::Int, sizeof(int), offsetof(GlobalUniforms, useDirLight), 0, 0, globalUniformsFlags},
			{"pointLights", Shader::ShaderUniformType::Unknown, sizeof(PointLight) * 4, offsetof(GlobalUniforms, pointLights), 0, 0, globalUniformsFlags},
			{"dirLights", Shader::ShaderUniformType::Unknown, sizeof(DirectionalLight) * 4, offsetof(GlobalUniforms, dirLights), 0, 0, globalUniformsFlags},
		};

		struct ObjectUniforms {
			glm::mat4 model;

			glm::vec4 albedo;
			float roughness;
			float metallic;
			float ao;

			int has_albedo_texture;
			int has_normal_texture;
			int has_roughness_texture;
			int has_metallic_texture;
			int has_ao_texture;
		};

		constexpr Shader::ShaderStageFlags objectUniformsFlags = Shader::StageToBit(Shader::ShaderStageType::Vertex) | Shader::StageToBit(Shader::ShaderStageType::Fragment);

		desc.objectUniforms = {
			{"model",			Shader::ShaderUniformType::Mat4, sizeof(glm::mat4), offsetof(ObjectUniforms, model), 1, 0, objectUniformsFlags},

			{"albedo",	Shader::ShaderUniformType::Vec4, sizeof(glm::vec4), offsetof(ObjectUniforms, albedo), 1, 0, objectUniformsFlags},
			{"roughness",	Shader::ShaderUniformType::Float,	sizeof(float), offsetof(ObjectUniforms, roughness), 1, 0, objectUniformsFlags},
			{"metallic",	Shader::ShaderUniformType::Float,	sizeof(float), offsetof(ObjectUniforms, metallic), 1, 0, objectUniformsFlags},
			{"ao",	Shader::ShaderUniformType::Float,	sizeof(float), offsetof(ObjectUniforms, ao), 1, 0, objectUniformsFlags},

			{"has_albedo_texture",	Shader::ShaderUniformType::Int,	sizeof(int), offsetof(ObjectUniforms, has_albedo_texture), 1, 0, objectUniformsFlags},
			{"has_normal_texture",	Shader::ShaderUniformType::Int,	sizeof(int), offsetof(ObjectUniforms, has_normal_texture), 1, 0, objectUniformsFlags},
			{"has_roughness_texture",	Shader::ShaderUniformType::Int,	sizeof(int), offsetof(ObjectUniforms, has_roughness_texture), 1, 0, objectUniformsFlags},
			{"has_metallic_texture",	Shader::ShaderUniformType::Int,	sizeof(int), offsetof(ObjectUniforms, has_metallic_texture), 1, 0, objectUniformsFlags},
			{"has_ao_texture",	Shader::ShaderUniformType::Int,	sizeof(int), offsetof(ObjectUniforms, has_ao_texture), 1, 0, objectUniformsFlags},
		};

		desc.samplers = {
			{"albedo_texture", 1, 1, Shader::StageToBit(Shader::ShaderStageType::Fragment)},
			{"normal_texture", 1, 2, Shader::StageToBit(Shader::ShaderStageType::Fragment)},
			{"roughness_texture", 1, 3, Shader::StageToBit(Shader::ShaderStageType::Fragment)},
			{"metallic_texture", 1, 4, Shader::StageToBit(Shader::ShaderStageType::Fragment)},
			{"ao_texture", 1, 5, Shader::StageToBit(Shader::ShaderStageType::Fragment)}
		};

		desc.blendMode = Shader::BlendMode::None;
		desc.cullMode = Shader::CullMode::Back;
		desc.fillMode = Shader::FillMode::Solid;
		desc.depthFunc = Shader::DepthFunc::Less;
		desc.depthTestEnable = true;
		desc.depthWriteEnable = true;
		desc.topology = Shader::PrimitiveTopology::TriangleList;
		desc.enableDynamicViewport = true;
		desc.enableDynamicScissor = true;
		desc.enableDynamicLineWidth = false;

		m_SceneData.m_Shader = Shader::Create(desc);

		m_SceneData.m_AssetManager = std::make_unique<AssetManager>();

		m_SceneData.m_Skybox = BasicSkybox::CreateBasicSkybox();

		m_SceneData.m_Skybox->LoadCubemap({
			"Assets/Textures/Skybox/right.jpg",   // +X
			"Assets/Textures/Skybox/left.jpg",    // -X
			"Assets/Textures/Skybox/top.jpg",     // +Y
			"Assets/Textures/Skybox/bottom.jpg",  // -Y
			"Assets/Textures/Skybox/front.jpg",   // +Z
			"Assets/Textures/Skybox/back.jpg"     // -Z
		});

		m_SceneData.m_ScriptSystem = std::make_unique<ScriptSystem>();
		m_SceneData.m_ScriptSystem->Initialize();

		m_SceneData.m_PointsBuffer.fill(PointLight());
		m_SceneData.m_DirectionalsBuffer.fill(DirectionalLight());
	}

	void Renderer::Shutdown()
	{
		m_SceneData.m_Skybox.reset();
		m_SceneData.m_Shader.reset();
		m_SceneData.m_AssetManager.reset();
		m_SceneData.m_ScriptSystem.reset();
	}

	void Renderer::BeginScene(Scene& scene)
	{
		m_SceneData.m_Scene = &scene;
	}

	void Renderer::Render(BaseCamera& camera)
	{
		Math::Frustum frustum = Math::CalculateFrustum(camera.getProjectionMatrix() * camera.getViewMatrix());

		glm::mat4 viewMatrix = camera.getViewMatrix();
		glm::mat4 projectionMatrix = camera.getProjectionMatrix();

		m_SceneData.m_Shader->Use();

		//int totalEntity = 0;
		//int entityDraw = 0;

		m_SceneData.m_Shader->SetUniform("view", &viewMatrix, sizeof(glm::mat4));
		m_SceneData.m_Shader->SetUniform("projection", &projectionMatrix, sizeof(glm::mat4));
		m_SceneData.m_Shader->SetUniform("camera_position", &camera.GetPosition(), sizeof(glm::vec3));

		m_SceneData.nDirs = 0;
		m_SceneData.nPts = 0;

		for (auto e : m_SceneData.m_Scene->GetAllEntitiesWith<LightComponent, TransformComponent>())
		{
			Entity entity{ e, m_SceneData.m_Scene->GetRegistry()};

			auto& lc = entity.GetComponent<LightComponent>();
			auto& tr = entity.GetComponent<TransformComponent>();

			if (lc.lightType == LightComponent::LightType::DIRECTIONAL && m_SceneData.nDirs < 4)
			{
				DirectionalLight dl = lc.directional_light;
				dl.direction = -Math::ForwardFromEulerRad(tr.Rotation);
				m_SceneData.m_DirectionalsBuffer[m_SceneData.nDirs++] = dl;
			}
			else if (lc.lightType == LightComponent::LightType::POINT && m_SceneData.nPts < 4)
			{
				PointLight pl = lc.point_light;
				pl.position = tr.Position;
				m_SceneData.m_PointsBuffer[m_SceneData.nPts++] = pl;
			}
		}

		m_SceneData.m_Shader->SetUniform("usePointLight", &m_SceneData.nPts, sizeof(int));
		m_SceneData.m_Shader->SetUniform("useDirLight", &m_SceneData.nDirs, sizeof(int));

		m_SceneData.m_Shader->SetUniform("pointLights", m_SceneData.m_PointsBuffer.data(), sizeof(PointLight) * 4);
		m_SceneData.m_Shader->SetUniform("dirLights", m_SceneData.m_DirectionalsBuffer.data(), sizeof(DirectionalLight) * 4);

		if (!m_SceneData.m_Shader->UpdateGlobalState())
		{
			return;
		}

		for (auto e : m_SceneData.m_Scene->GetAllEntitiesWith<TransformComponent, MaterialComponent, MeshComponent, MeshRendererComponent>())
		{
			Entity entity{ e, m_SceneData.m_Scene->GetRegistry()};

			auto& tr = entity.GetComponent<TransformComponent>();

			glm::mat4 transform = tr.GetGlobalTransform();

			auto& mc = entity.GetComponent<MeshComponent>();
			auto& matc = entity.GetComponent<MaterialComponent>();
			auto& mr = entity.GetComponent<MeshRendererComponent>();

			//totalEntity++;

			if (!mc.HasMesh())
			{
				continue;
			}

			if (!mr.m_Rendered)
			{
				continue;
			}

			if (!mc.GetMesh().IsVisible(frustum, transform, tr.Scale))
			{
				continue;
			}

			//entityDraw++;

			Material& material = matc.GetMaterial();

			m_SceneData.m_Shader->SetUniform("model", &transform, sizeof(glm::mat4));

			m_SceneData.m_Shader->SetUniform("albedo", &material.GetAlbedo(), sizeof(glm::vec4));
			m_SceneData.m_Shader->SetUniform("roughness", &material.GetRoughness(), sizeof(float));
			m_SceneData.m_Shader->SetUniform("metallic", &material.GetMetallic(), sizeof(float));
			m_SceneData.m_Shader->SetUniform("ao", &material.GetAO(), sizeof(float));

			int hasAlbedoTexture = material.HasTexture(Albedo) ? 1 : 0;
			int hasNormalTexture = material.HasTexture(Normal) ? 1 : 0;
			int hasRougnessTexture = material.HasTexture(Roughness) ? 1 : 0;
			int hasMetallicTexture = material.HasTexture(Metallic) ? 1 : 0;
			int hasAOTexture = material.HasTexture(AO) ? 1 : 0;

			m_SceneData.m_Shader->SetUniform("has_albedo_texture", &hasAlbedoTexture, sizeof(int));
			m_SceneData.m_Shader->SetUniform("has_normal_texture", &hasNormalTexture, sizeof(int));
			m_SceneData.m_Shader->SetUniform("has_roughness_texture", &hasRougnessTexture, sizeof(int));
			m_SceneData.m_Shader->SetUniform("has_metallic_texture", &hasMetallicTexture, sizeof(int));
			m_SceneData.m_Shader->SetUniform("has_ao_texture", &hasAOTexture, sizeof(int));

			m_SceneData.m_Shader->SetTexture("albedo_texture", material.GetTexture(Albedo));
			m_SceneData.m_Shader->SetTexture("normal_texture", material.GetTexture(Normal));
			m_SceneData.m_Shader->SetTexture("roughness_texture", material.GetTexture(Roughness));
			m_SceneData.m_Shader->SetTexture("metallic_texture", material.GetTexture(Metallic));
			m_SceneData.m_Shader->SetTexture("ao_texture", material.GetTexture(AO));

			if (!m_SceneData.m_Shader->UpdateObject(&material))
			{
				continue;
			}

			mc.GetMesh().draw();

			m_SceneData.m_Shader->Reset();
		}

		//std::cout << entityDraw << "/" << totalEntity << std::endl;
	}

	void Renderer::RenderSkybox(BaseCamera& camera)
	{
		m_SceneData.m_Skybox->Bind();

		glm::mat4 viewMatrix = camera.getViewMatrix();
		glm::mat4 projectionMatrix = camera.getProjectionMatrix();

		m_SceneData.m_Skybox->GetShader()->SetUniform("view", &viewMatrix, sizeof(glm::mat4));
		m_SceneData.m_Skybox->GetShader()->SetUniform("projection", &projectionMatrix, sizeof(glm::mat4));

		m_SceneData.m_Skybox->GetShader()->UpdateGlobalState();

		m_SceneData.m_Skybox->GetShader()->SetTexture("skybox", m_SceneData.m_Skybox->GetMaterial()->GetTexture(Albedo), Shader::SamplerType::SamplerCube);

		m_SceneData.m_Skybox->GetShader()->UpdateObject(m_SceneData.m_Skybox->GetMaterial());

		m_SceneData.m_Skybox->Draw();

		m_SceneData.m_Skybox->Unbind();
	}

	void Renderer::EndScene()
	{

	}

	Scene* Renderer::GetScene()
	{
		return m_SceneData.m_Scene;
	}

	double Renderer::GetTime()
	{
		return glfwGetTime();
	}

	RendererAPI::API Renderer::GetAPI()
	{
		return RendererAPI::GetAPI();
	}
}
