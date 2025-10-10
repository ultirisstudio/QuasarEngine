#include "qepch.h"

#include <GLFW/glfw3.h>

#include <glm/gtc/type_ptr.hpp>

#include <QuasarEngine/Renderer/Renderer.h>

#include <QuasarEngine/Entity/Entity.h>
#include <QuasarEngine/Resources/Materials/Material.h>
#include <QuasarEngine/Resources/Mesh.h>

#include <QuasarEngine/Entity/AllComponents.h>

#include <QuasarEngine/Tools/Math.h>

#include <QuasarEngine/UI/UIContainer.h>
#include <QuasarEngine/UI/UIText.h>
#include <QuasarEngine/UI/UIButton.h>

#include <QuasarEngine/Renderer/RenderCommand.h>
#include <QuasarEngine/Renderer/RendererAPI.h>
#include <QuasarEngine/Physic/PhysicEngine.h>

namespace QuasarEngine
{
	void Renderer::Initialize()
	{
		Shader::ShaderDescription desc;

		std::string basePath;
		std::string vertExt;
		std::string fragExt;

		if (RendererAPI::GetAPI() == RendererAPI::API::Vulkan)
		{
			basePath = "Assets/Shaders/vk/spv/";
			vertExt = ".vert.spv";
			fragExt = ".frag.spv";
		}
		else
		{
			basePath = "Assets/Shaders/gl/";
			vertExt = ".vert.glsl";
			fragExt = ".frag.glsl";
		}

		std::string vertPath = basePath + "basic" + vertExt;
		std::string fragPath = basePath + "basic" + fragExt;

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

		Shader::ShaderDescription phyDebDesc;

		std::string phyDebVertExt;
		std::string phyDebFragExt;

		if (RendererAPI::GetAPI() == RendererAPI::API::Vulkan)
		{
			phyDebVertExt = ".vert.spv";
			phyDebFragExt = ".frag.spv";
		}
		else
		{
			phyDebVertExt = ".vert.glsl";
			phyDebFragExt = ".frag.glsl";
		}

		std::string phyDebVertPath = basePath + "debug" + phyDebVertExt;
		std::string phyDebFragPath = basePath + "debug" + phyDebFragExt;

		phyDebDesc.modules = {
			Shader::ShaderModuleInfo{
				Shader::ShaderStageType::Vertex,
				phyDebVertPath,
				{
					{0, Shader::ShaderIOType::Vec3, "inPosition", true, ""},
					{1, Shader::ShaderIOType::Vec3, "inColor",    true, ""},
				}
			},
			Shader::ShaderModuleInfo{
				Shader::ShaderStageType::Fragment,
				phyDebFragPath,
				{}
			}
		};

		struct alignas(16) PhyDebGlobalUniforms
		{
			glm::mat4 view;
			glm::mat4 projection;
		};

		Shader::ShaderStageFlags phyDebGlobalUniformsFlags = Shader::StageToBit(Shader::ShaderStageType::Vertex) | Shader::StageToBit(Shader::ShaderStageType::Fragment);

		phyDebDesc.globalUniforms = {
			{"view", Shader::ShaderUniformType::Mat4, sizeof(glm::mat4), offsetof(PhyDebGlobalUniforms, view), 0, 0, phyDebGlobalUniformsFlags},
			{"projection", Shader::ShaderUniformType::Mat4, sizeof(glm::mat4), offsetof(PhyDebGlobalUniforms, projection), 0, 0, phyDebGlobalUniformsFlags}
		};

		struct PhyDebObjectUniforms {
			glm::mat4 model;
		};

		constexpr Shader::ShaderStageFlags phyDebObjectUniformsFlags = Shader::StageToBit(Shader::ShaderStageType::Vertex) | Shader::StageToBit(Shader::ShaderStageType::Fragment);

		phyDebDesc.objectUniforms = {
			{"model",			Shader::ShaderUniformType::Mat4, sizeof(glm::mat4), offsetof(PhyDebObjectUniforms, model), 1, 0, phyDebObjectUniformsFlags},
		};

		phyDebDesc.samplers = {
			
		};

		phyDebDesc.blendMode = Shader::BlendMode::None;
		phyDebDesc.cullMode = Shader::CullMode::None;
		phyDebDesc.fillMode = Shader::FillMode::Solid;
		phyDebDesc.depthFunc = Shader::DepthFunc::Always;
		phyDebDesc.depthTestEnable = false;
		phyDebDesc.depthWriteEnable = false;
		phyDebDesc.topology = Shader::PrimitiveTopology::LineList;
		phyDebDesc.enableDynamicViewport = true;
		phyDebDesc.enableDynamicScissor = true;
		phyDebDesc.enableDynamicLineWidth = false;

		m_SceneData.m_PhysicDebugShader = Shader::Create(phyDebDesc);

		Shader::ShaderDescription terrainDesc;

		terrainDesc.cullMode = Shader::CullMode::Back;

		std::string tVertPath = basePath + "gpuheight.vs";
		std::string tTcsPath = basePath + "gpuheight.tcs";
		std::string tTesPath = basePath + "gpuheight.tes";
		std::string tFragPath = basePath + "gpuheight.fs";

		terrainDesc.modules = {
			Shader::ShaderModuleInfo{
				Shader::ShaderStageType::Vertex,
				tVertPath,
				{
					{0, Shader::ShaderIOType::Vec3, "inPosition", true, ""},
					{1, Shader::ShaderIOType::Vec3, "inNormal", true, ""},
					{2, Shader::ShaderIOType::Vec2, "inTexCoord", true, ""}
				}
			},
			Shader::ShaderModuleInfo{
				Shader::ShaderStageType::TessControl,
				tTcsPath,
				{}
			},
			Shader::ShaderModuleInfo{
				Shader::ShaderStageType::TessEval,
				tTesPath,
				{}
			},
			Shader::ShaderModuleInfo{
				Shader::ShaderStageType::Fragment,
				tFragPath,
				{}
			}
		};

		struct alignas(16) TerrainGlobalUniforms {
			glm::mat4 view;
			glm::mat4 projection;
		};
		terrainDesc.globalUniforms = {
			{"view",       Shader::ShaderUniformType::Mat4,  sizeof(glm::mat4), offsetof(TerrainGlobalUniforms, view),       0, 0, Shader::StageToBit(Shader::ShaderStageType::Vertex) | Shader::StageToBit(Shader::ShaderStageType::TessEval)},
			{"projection", Shader::ShaderUniformType::Mat4,  sizeof(glm::mat4), offsetof(TerrainGlobalUniforms, projection), 0, 0, Shader::StageToBit(Shader::ShaderStageType::Vertex) | Shader::StageToBit(Shader::ShaderStageType::TessEval)},
		};

		struct TerrainObjectUniforms {
			glm::mat4 model;
			float heightMult;
			int   uTextureScale;
		};

		constexpr auto TOFlags = Shader::StageToBit(Shader::ShaderStageType::TessEval) | Shader::StageToBit(Shader::ShaderStageType::Fragment);

		terrainDesc.objectUniforms = {
			{"model",          Shader::ShaderUniformType::Mat4,  sizeof(glm::mat4),          offsetof(TerrainObjectUniforms, model),       1, 0, TOFlags},
			{"heightMult",     Shader::ShaderUniformType::Float, sizeof(float),              offsetof(TerrainObjectUniforms, heightMult),  1, 0, TOFlags},
			{"uTextureScale",  Shader::ShaderUniformType::Int,   sizeof(int),                offsetof(TerrainObjectUniforms, uTextureScale),1, 0, TOFlags},
		};

		terrainDesc.samplers = {
			{"heightMap", 1, 0, Shader::StageToBit(Shader::ShaderStageType::TessEval)},
			{"uTexture",  1, 1, Shader::StageToBit(Shader::ShaderStageType::Fragment)},
		};

		terrainDesc.blendMode = Shader::BlendMode::None;
		terrainDesc.cullMode = Shader::CullMode::Back;
		terrainDesc.fillMode = Shader::FillMode::Solid;
		terrainDesc.depthFunc = Shader::DepthFunc::Less;
		terrainDesc.depthTestEnable = true;
		terrainDesc.depthWriteEnable = true;

		terrainDesc.topology = Shader::PrimitiveTopology::PatchList;

		terrainDesc.patchControlPoints = 4;

		terrainDesc.enableDynamicViewport = true;
		terrainDesc.enableDynamicScissor = true;

		m_SceneData.m_TerrainShader = Shader::Create(terrainDesc);

		Shader::ShaderDescription skinnedDesc;

		std::string sVertPath = basePath + "basic_anim" + vertExt;
		std::string sFragPath = basePath + "basic_anim" + fragExt;

		skinnedDesc.modules = {
			Shader::ShaderModuleInfo{
				Shader::ShaderStageType::Vertex,
				sVertPath,
				{
					{0, Shader::ShaderIOType::Vec3,  "inPosition", true, ""},
					{1, Shader::ShaderIOType::Vec3,  "inNormal",   true, ""},
					{2, Shader::ShaderIOType::Vec2,  "inTexCoord", true, ""},
					{3, Shader::ShaderIOType::IVec4, "inBoneIds",  true, ""},
					{4, Shader::ShaderIOType::Vec4,  "inWeights",  true, ""},
				}
			},
			Shader::ShaderModuleInfo{
				Shader::ShaderStageType::Fragment,
				sFragPath,
				{}
			}
		};

		struct alignas(16) SkinnedGlobalUniforms {
			glm::mat4 view;
			glm::mat4 projection;
			glm::vec3 camera_position;
			int usePointLight;
			int useDirLight;
			PointLight pointLights[4];
			DirectionalLight dirLights[4];
		};

		constexpr auto SkinnedGlobalStages = Shader::StageToBit(Shader::ShaderStageType::Vertex) | Shader::StageToBit(Shader::ShaderStageType::Fragment);

		skinnedDesc.globalUniforms = {
			{"view", Shader::ShaderUniformType::Mat4, sizeof(glm::mat4), offsetof(SkinnedGlobalUniforms, view), 0, 0, SkinnedGlobalStages},
			{"projection", Shader::ShaderUniformType::Mat4, sizeof(glm::mat4), offsetof(SkinnedGlobalUniforms, projection), 0, 0, SkinnedGlobalStages},
			{"camera_position", Shader::ShaderUniformType::Vec3, sizeof(glm::vec3), offsetof(SkinnedGlobalUniforms, camera_position), 0, 0, SkinnedGlobalStages},
			{"usePointLight", Shader::ShaderUniformType::Int, sizeof(int), offsetof(SkinnedGlobalUniforms, usePointLight), 0, 0, SkinnedGlobalStages},
			{"useDirLight", Shader::ShaderUniformType::Int, sizeof(int), offsetof(SkinnedGlobalUniforms, useDirLight), 0, 0, SkinnedGlobalStages},
			{"pointLights", Shader::ShaderUniformType::Unknown, sizeof(PointLight) * 4, offsetof(SkinnedGlobalUniforms, pointLights), 0, 0, SkinnedGlobalStages},
			{"dirLights", Shader::ShaderUniformType::Unknown, sizeof(DirectionalLight) * 4, offsetof(SkinnedGlobalUniforms, dirLights), 0, 0, SkinnedGlobalStages},
		};

		struct alignas(16) SkinnedObjectUniforms {
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

			glm::mat4 finalBonesMatrices[QE_MAX_BONES];
		};

		skinnedDesc.objectUniforms = {
			{"model",			Shader::ShaderUniformType::Mat4, sizeof(glm::mat4), offsetof(SkinnedObjectUniforms, model), 1, 0, SkinnedGlobalStages},

			{"albedo",	Shader::ShaderUniformType::Vec4, sizeof(glm::vec4), offsetof(SkinnedObjectUniforms, albedo), 1, 0, SkinnedGlobalStages},
			{"roughness",	Shader::ShaderUniformType::Float,	sizeof(float), offsetof(SkinnedObjectUniforms, roughness), 1, 0, SkinnedGlobalStages},
			{"metallic",	Shader::ShaderUniformType::Float,	sizeof(float), offsetof(SkinnedObjectUniforms, metallic), 1, 0, SkinnedGlobalStages},
			{"ao",	Shader::ShaderUniformType::Float,	sizeof(float), offsetof(SkinnedObjectUniforms, ao), 1, 0, SkinnedGlobalStages},

			{"has_albedo_texture",	Shader::ShaderUniformType::Int,	sizeof(int), offsetof(SkinnedObjectUniforms, has_albedo_texture), 1, 0, SkinnedGlobalStages},
			{"has_normal_texture",	Shader::ShaderUniformType::Int,	sizeof(int), offsetof(SkinnedObjectUniforms, has_normal_texture), 1, 0, SkinnedGlobalStages},
			{"has_roughness_texture",	Shader::ShaderUniformType::Int,	sizeof(int), offsetof(SkinnedObjectUniforms, has_roughness_texture), 1, 0, SkinnedGlobalStages},
			{"has_metallic_texture",	Shader::ShaderUniformType::Int,	sizeof(int), offsetof(SkinnedObjectUniforms, has_metallic_texture), 1, 0, SkinnedGlobalStages},
			{"has_ao_texture",	Shader::ShaderUniformType::Int,	sizeof(int), offsetof(SkinnedObjectUniforms, has_ao_texture), 1, 0, SkinnedGlobalStages},

			{"finalBonesMatrices",  Shader::ShaderUniformType::Unknown,sizeof(glm::mat4) * QE_MAX_BONES, offsetof(SkinnedObjectUniforms, finalBonesMatrices),  1, 0, SkinnedGlobalStages},
		};

		skinnedDesc.samplers = {
			{"albedo_texture", 1, 1, Shader::StageToBit(Shader::ShaderStageType::Fragment)},
			{"normal_texture", 1, 2, Shader::StageToBit(Shader::ShaderStageType::Fragment)},
			{"roughness_texture", 1, 3, Shader::StageToBit(Shader::ShaderStageType::Fragment)},
			{"metallic_texture", 1, 4, Shader::StageToBit(Shader::ShaderStageType::Fragment)},
			{"ao_texture", 1, 5, Shader::StageToBit(Shader::ShaderStageType::Fragment)}
		};

		skinnedDesc.blendMode = Shader::BlendMode::None;
		skinnedDesc.cullMode = Shader::CullMode::Back;
		skinnedDesc.fillMode = Shader::FillMode::Solid;
		skinnedDesc.depthFunc = Shader::DepthFunc::Less;
		skinnedDesc.depthTestEnable = true;
		skinnedDesc.depthWriteEnable = true;
		skinnedDesc.topology = Shader::PrimitiveTopology::TriangleList;
		skinnedDesc.enableDynamicViewport = true;
		skinnedDesc.enableDynamicScissor = true;

		m_SceneData.m_SkinnedShader = Shader::Create(skinnedDesc);

		for (int i = 0; i < QE_MAX_BONES; ++i)
			m_SceneData.m_IdentityBones[i] = glm::mat4(1.0f);

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

		m_SceneData.m_UI = std::make_unique<UISystem>();

		auto root = std::make_shared<UIContainer>("Root");
		root->layout = UILayoutType::Vertical;
		root->Style().padding = 12.f;
		root->Style().bg = { 0.1f, 0.1f, 0.1f, 0.8f };
		root->Transform().pos = { 10.f, 10.f };
		root->Transform().size = { 500.f, 100.f };

		auto title = std::make_shared<UIText>("Title");
		title->text = "Menu Pause";
		title->Style().bg = { 0,0,0,0 };
		title->Transform().pos = { 0.f, 0.f };
		title->Transform().size = { 10.f, 50.f };
		root->AddChild(title);

		auto btn = std::make_shared<UIButton>("Resume");
		btn->label = "Reprendre";
		btn->onClick = []() { std::cout << "Resume clicked\n"; };
		btn->Transform().pos = { 0.f, 0.f };
		btn->Transform().size = { 100.f, 50.f };
		root->AddChild(btn);

		m_SceneData.m_UI->SetRoot(root);
	}

	void Renderer::Shutdown()
	{
		m_SceneData.m_Skybox.reset();
		m_SceneData.m_Shader.reset();
		m_SceneData.m_PhysicDebugShader.reset();
		m_SceneData.m_UI.reset();
		m_SceneData.m_ScriptSystem.reset();
	}

	void Renderer::BeginScene(Scene& scene)
	{
		m_SceneData.m_Scene = &scene;
	}

	void Renderer::Render(BaseCamera& camera)
	{
		auto FindAnimatorForEntity = [&](Entity e) -> AnimationComponent*
			{
				if (e.HasComponent<AnimationComponent>())
					return &e.GetComponent<AnimationComponent>();

				if (e.HasComponent<HierarchyComponent>()) {
					const auto& h = e.GetComponent<HierarchyComponent>();
					if (h.m_Parent != UUID::Null()) {
						if (auto parentOpt = m_SceneData.m_Scene->GetEntityByUUID(h.m_Parent)) {
							if (parentOpt->HasComponent<AnimationComponent>())
								return &parentOpt->GetComponent<AnimationComponent>();
						}
					}
				}
				return nullptr;
			};

		const glm::mat4 VP = camera.getProjectionMatrix() * camera.getViewMatrix();
		Math::Frustum frustum = Math::CalculateFrustum(VP);

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
			Entity entity{ e, m_SceneData.m_Scene->GetRegistry() };

			auto& tr = entity.GetComponent<TransformComponent>();
			auto& mc = entity.GetComponent<MeshComponent>();
			auto& matc = entity.GetComponent<MaterialComponent>();
			auto& mr = entity.GetComponent<MeshRendererComponent>();

			if (!mr.m_Rendered || !mc.HasMesh()) continue;

			if (mc.GetMesh().HasSkinning()) continue;

			glm::mat4 model = tr.GetGlobalTransform();
			if (mc.HasLocalNodeTransform()) model *= mc.GetLocalNodeTransform();

			if (!mc.GetMesh().IsVisible(frustum, model)) {
				// continue;
			}

			Material& material = matc.GetMaterial();

			m_SceneData.m_Shader->SetUniform("model", &model, sizeof(glm::mat4));

			m_SceneData.m_Shader->SetUniform("albedo", &material.GetAlbedo(), sizeof(glm::vec4));
			float rough = material.GetRoughness();
			float metal = material.GetMetallic();
			float ao = material.GetAO();
			m_SceneData.m_Shader->SetUniform("roughness", &rough, sizeof(float));
			m_SceneData.m_Shader->SetUniform("metallic", &metal, sizeof(float));
			m_SceneData.m_Shader->SetUniform("ao", &ao, sizeof(float));

			int hasA = material.HasTexture(Albedo) ? 1 : 0;
			int hasN = material.HasTexture(Normal) ? 1 : 0;
			int hasR = material.HasTexture(Roughness) ? 1 : 0;
			int hasM = material.HasTexture(Metallic) ? 1 : 0;
			int hasO = material.HasTexture(AO) ? 1 : 0;

			m_SceneData.m_Shader->SetUniform("has_albedo_texture", &hasA, sizeof(int));
			m_SceneData.m_Shader->SetUniform("has_normal_texture", &hasN, sizeof(int));
			m_SceneData.m_Shader->SetUniform("has_roughness_texture", &hasR, sizeof(int));
			m_SceneData.m_Shader->SetUniform("has_metallic_texture", &hasM, sizeof(int));
			m_SceneData.m_Shader->SetUniform("has_ao_texture", &hasO, sizeof(int));

			m_SceneData.m_Shader->SetTexture("albedo_texture", material.GetTexture(Albedo));
			m_SceneData.m_Shader->SetTexture("normal_texture", material.GetTexture(Normal));
			m_SceneData.m_Shader->SetTexture("roughness_texture", material.GetTexture(Roughness));
			m_SceneData.m_Shader->SetTexture("metallic_texture", material.GetTexture(Metallic));
			m_SceneData.m_Shader->SetTexture("ao_texture", material.GetTexture(AO));

			if (!m_SceneData.m_Shader->UpdateObject(&material)) continue;

			mc.GetMesh().draw();

			m_SceneData.m_Shader->Reset();
		}

		m_SceneData.m_Shader->Unuse();

		m_SceneData.m_SkinnedShader->Use();

		m_SceneData.m_SkinnedShader->SetUniform("view", &viewMatrix, sizeof(glm::mat4));
		m_SceneData.m_SkinnedShader->SetUniform("projection", &projectionMatrix, sizeof(glm::mat4));
		m_SceneData.m_SkinnedShader->SetUniform("camera_position", &camera.GetPosition(), sizeof(glm::vec3));

		m_SceneData.m_SkinnedShader->SetUniform("usePointLight", &m_SceneData.nPts, sizeof(int));
		m_SceneData.m_SkinnedShader->SetUniform("useDirLight", &m_SceneData.nDirs, sizeof(int));

		m_SceneData.m_SkinnedShader->SetUniform("pointLights", m_SceneData.m_PointsBuffer.data(), sizeof(PointLight) * 4);
		m_SceneData.m_SkinnedShader->SetUniform("dirLights", m_SceneData.m_DirectionalsBuffer.data(), sizeof(DirectionalLight) * 4);

		if (!m_SceneData.m_SkinnedShader->UpdateGlobalState())
		{
			return;
		}

		for (auto e : m_SceneData.m_Scene->GetAllEntitiesWith<TransformComponent, MaterialComponent, MeshComponent, MeshRendererComponent>())
		{
			Entity entity{ e, m_SceneData.m_Scene->GetRegistry() };

			auto& tr = entity.GetComponent<TransformComponent>();
			auto& mc = entity.GetComponent<MeshComponent>();
			auto& matc = entity.GetComponent<MaterialComponent>();
			auto& mr = entity.GetComponent<MeshRendererComponent>();

			if (!mr.m_Rendered || !mc.HasMesh()) continue;

			if (!mc.GetMesh().HasSkinning()) continue;

			glm::mat4 model = tr.GetGlobalTransform();

			if (!mc.GetMesh().IsVisible(frustum, model)) {
				// continue;
			}

			m_SceneData.m_SkinnedShader->SetUniform("model", &model, sizeof(glm::mat4));

			Material& material = matc.GetMaterial();

			m_SceneData.m_SkinnedShader->SetUniform("albedo", &material.GetAlbedo(), sizeof(glm::vec4));
			float rough = material.GetRoughness();
			float metal = material.GetMetallic();
			float ao = material.GetAO();
			m_SceneData.m_SkinnedShader->SetUniform("roughness", &rough, sizeof(float));
			m_SceneData.m_SkinnedShader->SetUniform("metallic", &metal, sizeof(float));
			m_SceneData.m_SkinnedShader->SetUniform("ao", &ao, sizeof(float));

			int hasA = material.HasTexture(Albedo) ? 1 : 0;
			int hasN = material.HasTexture(Normal) ? 1 : 0;
			int hasR = material.HasTexture(Roughness) ? 1 : 0;
			int hasM = material.HasTexture(Metallic) ? 1 : 0;
			int hasO = material.HasTexture(AO) ? 1 : 0;

			m_SceneData.m_SkinnedShader->SetUniform("has_albedo_texture", &hasA, sizeof(int));
			m_SceneData.m_SkinnedShader->SetUniform("has_normal_texture", &hasN, sizeof(int));
			m_SceneData.m_SkinnedShader->SetUniform("has_roughness_texture", &hasR, sizeof(int));
			m_SceneData.m_SkinnedShader->SetUniform("has_metallic_texture", &hasM, sizeof(int));
			m_SceneData.m_SkinnedShader->SetUniform("has_ao_texture", &hasO, sizeof(int));

			m_SceneData.m_SkinnedShader->SetTexture("albedo_texture", material.GetTexture(Albedo));
			m_SceneData.m_SkinnedShader->SetTexture("normal_texture", material.GetTexture(Normal));
			m_SceneData.m_SkinnedShader->SetTexture("roughness_texture", material.GetTexture(Roughness));
			m_SceneData.m_SkinnedShader->SetTexture("metallic_texture", material.GetTexture(Metallic));
			m_SceneData.m_SkinnedShader->SetTexture("ao_texture", material.GetTexture(AO));

			const AnimationComponent* anim = FindAnimatorForEntity(entity);
			if (anim && !anim->GetFinalBoneMatrices().empty()) {
				std::vector<glm::mat4> mats = anim->GetFinalBoneMatrices();
				const size_t n = std::min(mats.size(), (size_t)QE_MAX_BONES);
				if (n == (size_t)QE_MAX_BONES) {
					m_SceneData.m_SkinnedShader->SetUniform("finalBonesMatrices", mats.data(), sizeof(glm::mat4) * QE_MAX_BONES);
				}
				else {
					std::array<glm::mat4, QE_MAX_BONES> tmp = m_SceneData.m_IdentityBones;
					std::memcpy(tmp.data(), mats.data(), sizeof(glm::mat4) * n);
					m_SceneData.m_SkinnedShader->SetUniform("finalBonesMatrices", tmp.data(), sizeof(glm::mat4) * QE_MAX_BONES);
				}
			}
			else {
				m_SceneData.m_SkinnedShader->SetUniform("finalBonesMatrices",
					m_SceneData.m_IdentityBones.data(),
					sizeof(glm::mat4) * QE_MAX_BONES);
			}

			if (!m_SceneData.m_SkinnedShader->UpdateObject(&material)) continue;

			mc.GetMesh().draw();

			m_SceneData.m_SkinnedShader->Reset();
		}

		m_SceneData.m_SkinnedShader->Unuse();

		m_SceneData.m_TerrainShader->Use();

		m_SceneData.m_TerrainShader->SetUniform("view", &viewMatrix, sizeof(glm::mat4));
		m_SceneData.m_TerrainShader->SetUniform("projection", &projectionMatrix, sizeof(glm::mat4));

		if (!m_SceneData.m_TerrainShader->UpdateGlobalState())
		{
			return;
		}

		for (auto e : m_SceneData.m_Scene->GetAllEntitiesWith<TransformComponent, MaterialComponent, TerrainComponent, MeshRendererComponent>())
		{
			Entity entity{ e, m_SceneData.m_Scene->GetRegistry() };

			auto& tr = entity.GetComponent<TransformComponent>();
			auto& tc = entity.GetComponent<TerrainComponent>();
			auto& mr = entity.GetComponent<MeshRendererComponent>();
			auto& mat = entity.GetComponent<MaterialComponent>().GetMaterial();

			if (!mr.m_Rendered) continue;

			auto mesh = tc.GetMesh();
			if (!mesh || !mesh->IsMeshGenerated()) continue;

			glm::mat4 transform = tr.GetGlobalTransform();

			m_SceneData.m_TerrainShader->SetUniform("model", &transform, sizeof(glm::mat4));
			m_SceneData.m_TerrainShader->SetUniform("heightMult", &tc.heightMult, sizeof(float));
			m_SceneData.m_TerrainShader->SetUniform("uTextureScale", &tc.textureScale, sizeof(int));

			if (AssetManager::Instance().isAssetLoaded(tc.GetHeightMapPath()))
			{
				m_SceneData.m_TerrainShader->SetTexture("heightMap", AssetManager::Instance().getAsset<Texture2D>(tc.GetHeightMapPath()).get());
			}
			else
			{
				AssetToLoad tcAsset;
				tcAsset.id = tc.GetHeightMapPath();
				tcAsset.type = AssetType::TEXTURE;
				AssetManager::Instance().loadAsset(tcAsset);
			}

			m_SceneData.m_TerrainShader->SetTexture("uTexture", mat.GetTexture(Albedo));

			if (!m_SceneData.m_TerrainShader->UpdateObject(&mat)) {
				m_SceneData.m_TerrainShader->Unuse();
				continue;
			}

			//glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);

			mesh->draw();

			//glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);

			m_SceneData.m_TerrainShader->Reset();
		}

		m_SceneData.m_TerrainShader->Unuse();

		//std::cout << entityDraw << "/" << totalEntity << std::endl;

		/*
		if (entity.HasComponent<AnimationComponent>() && instIsSkinned) {
			auto& ac = entity.GetComponent<AnimationComponent>();
			const auto& mats = ac.GetFinalBoneMatrices();
			for (int i = 0; i < (int)mats.size(); ++i) {
				m_SceneData.m_Shader->SetUniform(
					("finalBonesMatrices[" + std::to_string(i) + "]").c_str(),
					&mats[i], sizeof(glm::mat4)
				);
			}
		}

		// Option A: animation sur l'entité elle-même
		QuasarEngine::AnimationComponent* anim = nullptr;
		if (entity.HasComponent<QuasarEngine::AnimationComponent>())
			anim = &entity.GetComponent<QuasarEngine::AnimationComponent>();
		else if (entity.HasComponent<HierarchyComponent>()) {
			// Option B: remonter au parent (animation mise sur l'entité racine du modèle)
			auto& h = entity.GetComponent<HierarchyComponent>();
			if (h.m_Parent != UUID::Null()) {
				auto parentOpt = m_SceneData.m_Scene->GetEntityByUUID(h.m_Parent);
				if (parentOpt && parentOpt->HasComponent<QuasarEngine::AnimationComponent>())
					anim = &parentOpt->GetComponent<QuasarEngine::AnimationComponent>();
			}
		}

		if (anim && mc.HasMesh() && mc.GetMesh().HasSkinning()) // HasSkinning() à exposer sur Mesh si pas déjà
		{
			const auto& bones = anim->GetFinalBoneMatrices();
			const int boneCount = (int)bones.size();
			// (optionnel) m_SceneData.m_Shader->SetUniform("boneCount", &boneCount, sizeof(int));
			for (int i = 0; i < boneCount; ++i)
				m_SceneData.m_Shader->SetUniform(
					("finalBonesMatrices[" + std::to_string(i) + "]").c_str(),
					&bones[i], sizeof(glm::mat4));
		}
		*/
	}

	void Renderer::RenderDebug(BaseCamera& camera)
	{
		if (PhysicEngine::Instance().GetDebugVertexArray())
		{
			glm::mat4 viewMatrix = camera.getViewMatrix();
			glm::mat4 projectionMatrix = camera.getProjectionMatrix();

			m_SceneData.m_PhysicDebugShader->Use();

			glm::mat4 model = glm::mat4(1.0f);

			m_SceneData.m_PhysicDebugShader->SetUniform("view", glm::value_ptr(viewMatrix), sizeof(glm::mat4));
			m_SceneData.m_PhysicDebugShader->SetUniform("projection", glm::value_ptr(projectionMatrix), sizeof(glm::mat4));
			m_SceneData.m_PhysicDebugShader->SetUniform("model", glm::value_ptr(model), sizeof(glm::mat4));

			m_SceneData.m_PhysicDebugShader->UpdateGlobalState();
			m_SceneData.m_PhysicDebugShader->UpdateObject(nullptr);

			PhysicEngine::Instance().GetDebugVertexArray()->Bind();

			RenderCommand::Instance().DrawArrays(DrawMode::LINES, PhysicEngine::Instance().GetDebugVertexArray()->GetVertexBuffers()[0]->GetSize() / sizeof(float) / 6); //vertices.size() / 6

			m_SceneData.m_PhysicDebugShader->Unuse();
		}
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

	void Renderer::RenderUI(BaseCamera& camera)
	{
		UIFBInfo fb{ 1920.0f, 1080.0f, 1.0 };
		m_SceneData.m_UI->Render(camera, fb);
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
}
