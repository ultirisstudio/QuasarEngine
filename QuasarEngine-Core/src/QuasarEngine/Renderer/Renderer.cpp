#include "qepch.h"

#include <GLFW/glfw3.h>

#include <glm/gtc/type_ptr.hpp>

#include <QuasarEngine/Renderer/Renderer.h>
#include <QuasarEngine/Renderer/Renderer2D.h>

#include <QuasarEngine/Entity/Entity.h>
#include <QuasarEngine/Resources/Materials/Material.h>
#include <QuasarEngine/Resources/Mesh.h>

#include <QuasarEngine/Entity/AllComponents.h>

#include <QuasarEngine/Tools/Math.h>

#include <QuasarEngine/UI/UISystem.h>

#include <QuasarEngine/Renderer/RenderCommand.h>
#include <QuasarEngine/Renderer/RendererAPI.h>
#include <QuasarEngine/Physic/PhysicEngine.h>
#include <limits>

#include <QuasarEngine/Renderer/RenderContext.h>
#include <QuasarEngine/Renderer/RenderObject.h>
#include <QuasarEngine/Renderer/PBRStaticTechnique.h>
#include <QuasarEngine/Renderer/PBRSkinTechnique.h>
#include <QuasarEngine/Renderer/TerrainTechnique.h>

namespace QuasarEngine
{
	void Renderer::Initialize()
	{
		auto extFor = [](RendererAPI::API api, Shader::ShaderStageType s) {
			if (api == RendererAPI::API::Vulkan) {
				switch (s) {
				case Shader::ShaderStageType::Vertex:     return ".vert.spv";
				case Shader::ShaderStageType::TessControl:return ".tesc.spv";
				case Shader::ShaderStageType::TessEval:   return ".tese.spv";
				case Shader::ShaderStageType::Fragment:   return ".frag.spv";
				default: return "";
				}
			}
			else {
				switch (s) {
				case Shader::ShaderStageType::Vertex:     return ".vert.glsl";
				case Shader::ShaderStageType::TessControl:return ".tesc.glsl";
				case Shader::ShaderStageType::TessEval:   return ".tese.glsl";
				case Shader::ShaderStageType::Fragment:   return ".frag.glsl";
				default: return "";
				}
			}
			};

		const auto api = RendererAPI::GetAPI();
		const std::string basePath = (api == RendererAPI::API::Vulkan)
			? "Assets/Shaders/vk/spv/"
			: "Assets/Shaders/gl/";

		/*Shader::ShaderDescription phyDebDesc;

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

		constexpr Shader::ShaderStageFlags phyDebGlobalUniformsFlags = Shader::StageToBit(Shader::ShaderStageType::Vertex) | Shader::StageToBit(Shader::ShaderStageType::Fragment);

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

		m_SceneData.m_PhysicDebugShader = Shader::Create(phyDebDesc);*/

		SkyboxHDR::Settings skyboxSettings;
		skyboxSettings.hdrPath = "Assets/HDR/kloofendal_48d_partly_cloudy_puresky_4k.hdr";
		m_SceneData.m_SkyboxHDR = std::make_shared<SkyboxHDR>(skyboxSettings);

		m_SceneData.m_ScriptSystem = std::make_unique<ScriptSystem>();
		m_SceneData.m_ScriptSystem->Initialize();

		m_SceneData.m_PointsBuffer.fill(PointLight());
		m_SceneData.m_DirectionalsBuffer.fill(DirectionalLight());

		m_SceneData.m_UI = std::make_unique<UISystem>();

		/* {
			Shader::ShaderDescription pcDesc;

			const std::string pcName = "point_cloud";
			std::string pcVertPath = basePath + pcName + extFor(api, Shader::ShaderStageType::Vertex);
			std::string pcFragPath = basePath + pcName + extFor(api, Shader::ShaderStageType::Fragment);

			pcDesc.modules = {
				Shader::ShaderModuleInfo{
					Shader::ShaderStageType::Vertex,
					pcVertPath,
					{
						{0, Shader::ShaderIOType::Vec3, "inPosition", true, ""},
						{1, Shader::ShaderIOType::Vec4, "inColor",    true, ""},
					}
				},
				Shader::ShaderModuleInfo{
					Shader::ShaderStageType::Fragment,
					pcFragPath,
					{}
				}
			};

			struct alignas(16) PointCloudGlobalUniforms
			{
				glm::mat4 view;
				glm::mat4 projection;

				float pointSize;
				float vertices[];
			};

			struct alignas(16) PointCloudObjectUniforms
			{
				glm::mat4 model;
			};

			constexpr Shader::ShaderStageFlags PCStages =
				Shader::StageToBit(Shader::ShaderStageType::Vertex) |
				Shader::StageToBit(Shader::ShaderStageType::Fragment);

			pcDesc.globalUniforms = {
				{"view",		Shader::ShaderUniformType::Mat4,	sizeof(glm::mat4),	offsetof(PointCloudGlobalUniforms, view),       0, 0, PCStages},
				{"projection",	Shader::ShaderUniformType::Mat4,	sizeof(glm::mat4),	offsetof(PointCloudGlobalUniforms, projection),	0, 0, PCStages},
				{"pointSize",	Shader::ShaderUniformType::Float,	sizeof(float),		offsetof(PointCloudGlobalUniforms, pointSize),  0, 0, PCStages}
			};

			pcDesc.objectUniforms = {
				{"model",	Shader::ShaderUniformType::Mat4,	sizeof(glm::mat4),	offsetof(PointCloudObjectUniforms, model)}
			};

			const size_t ssboSize = std::numeric_limits<size_t>::max();

			//Shader::ShaderStorageBufferDesc verticesSB{};
			//verticesSB.name = "vertices_buffer";
			//verticesSB.size = ssboSize;
			//verticesSB.binding = 2;
			//verticesSB.stages = Shader::StageToBit(Shader::ShaderStageType::Vertex);

			//pcDesc.storageBuffers.push_back(verticesSB);

			pcDesc.samplers = {};

			pcDesc.blendMode = Shader::BlendMode::None;
			pcDesc.cullMode = Shader::CullMode::None;
			pcDesc.fillMode = Shader::FillMode::Solid;
			pcDesc.depthFunc = Shader::DepthFunc::Less;
			pcDesc.depthTestEnable = true;
			pcDesc.depthWriteEnable = true;

			pcDesc.topology = Shader::PrimitiveTopology::PointList;
			pcDesc.enableDynamicViewport = true;
			pcDesc.enableDynamicScissor = true;
			pcDesc.enableDynamicLineWidth = false;

			m_SceneData.m_PointCloudShader = Shader::Create(pcDesc);
		}*/
	}

	void Renderer::Shutdown()
	{
		m_SceneData.m_SkyboxHDR.reset();
		//m_SceneData.m_PhysicDebugShader.reset();
		//m_SceneData.m_PointCloudShader.reset();
		m_SceneData.m_UI.reset();
		m_SceneData.m_ScriptSystem.reset();
	}

	void Renderer::BeginScene(Scene& scene)
	{
		m_SceneData.m_Scene = &scene;
	}

	void Renderer::Render(BaseCamera& camera)
	{
		RenderContext ctx;
		ctx.view = camera.getViewMatrix();
		ctx.projection = camera.getProjectionMatrix();
		ctx.cameraPosition = camera.GetPosition();

		ctx.numPointLights = m_SceneData.nPts;
		ctx.numDirLights = m_SceneData.nDirs;
		ctx.pointLights = m_SceneData.m_PointsBuffer.data();
		ctx.dirLights = m_SceneData.m_DirectionalsBuffer.data();

		ctx.scene = m_SceneData.m_Scene;
		ctx.skybox = m_SceneData.m_SkyboxHDR.get();

		auto& registry = m_SceneData.m_Scene->GetRegistry()->GetRegistry();
		auto group = registry.group<TransformComponent, MeshComponent,
			MaterialComponent, MeshRendererComponent>();

		std::vector<RenderObject> staticMeshes;
		std::vector<RenderObject> skinnedMeshes;
		std::vector<RenderObject> pointClouds;
		std::vector<RenderObject> terrains;

		for (auto [e, tr, mc, matc, mr] : group.each())
		{
			if (!mr.m_Rendered || !mc.HasMesh()) continue;

			RenderObject obj;
			obj.entity = Entity{ e, m_SceneData.m_Scene->GetRegistry() };
			obj.mesh = &mc.GetMesh();
			obj.material = &matc.GetMaterial();

			glm::mat4 model = tr.GetGlobalTransform();
			if (mc.HasLocalNodeTransform()) model *= mc.GetLocalNodeTransform();
			obj.model = model;

			if (mc.GetMesh().HasSkinning())
				obj.flags = obj.flags | RenderFlags::Skinned;
			if (mc.GetMesh().IsCloudPoint())
				obj.flags = obj.flags | RenderFlags::PointCloud;

			if (HasFlag(obj.flags, RenderFlags::PointCloud))
				pointClouds.push_back(obj);
			else if (HasFlag(obj.flags, RenderFlags::Skinned))
				skinnedMeshes.push_back(obj);
			else
				staticMeshes.push_back(obj);
		}

		for (auto [e, tr, tec, matc, mr] :
			registry.group<TransformComponent, TerrainComponent,
			MaterialComponent, MeshRendererComponent>().each())
		{
			if (!mr.m_Rendered || !tec.IsGenerated())
				continue;

			if (tec.UseQuadtree() && !tec.HasQuadtree())
				tec.BuildQuadtree();

			glm::mat4 model = tr.GetGlobalTransform();

			if (tec.UseQuadtree() && tec.HasQuadtree())
			{
				auto* qt = tec.GetQuadtree();
				std::vector<const TerrainQuadtree::Node*> visibleNodes;
				qt->CollectVisible(ctx.projection * ctx.view, ctx.cameraPosition, visibleNodes);

				for (const auto* node : visibleNodes)
				{
					if (!node->mesh)
						continue;

					RenderObject obj;
					obj.entity = Entity{ e, m_SceneData.m_Scene->GetRegistry() };
					obj.mesh = node->mesh.get();
					obj.material = &matc.GetMaterial();
					obj.model = model;
					obj.flags = obj.flags | RenderFlags::Terrain;

					terrains.push_back(obj);
				}
			}
			else
			{
				RenderObject obj;
				obj.entity = Entity{ e, m_SceneData.m_Scene->GetRegistry() };
				obj.mesh = tec.GetMesh().get();
				obj.material = &matc.GetMaterial();
				obj.model = model;
				obj.flags = obj.flags | RenderFlags::Terrain;
				terrains.push_back(obj);
			}
		}

		PBRStaticTechnique staticTech(m_SceneData.m_SkyboxHDR.get());
		PBRSkinTechnique skinnedTech(m_SceneData.m_SkyboxHDR.get());
		TerrainTechnique terrainTech(m_SceneData.m_SkyboxHDR.get());
		//PointCloudTechnique pcTech(m_SceneData.m_SkyboxHDR.get());

		staticTech.Begin(ctx);
		for (auto& obj : staticMeshes)
		{
			staticTech.Submit(ctx, obj);
		}
		staticTech.End();

		skinnedTech.Begin(ctx);
		for (auto& obj : skinnedMeshes)
		{
			skinnedTech.Submit(ctx, obj);
		}
		skinnedTech.End();

		terrainTech.Begin(ctx);
		for (auto& obj : terrains)
			terrainTech.Submit(ctx, obj);
		terrainTech.End();

		/*pcTech.Begin(ctx);
		for (auto& obj : pointClouds)
			pcTech.Submit(ctx, obj);
		pcTech.End();*/

		/*m_SceneData.m_PointCloudShader->Use();

		float pointSize = 5.0f;

		m_SceneData.m_PointCloudShader->SetUniform("view", &viewMatrix, sizeof(glm::mat4));
		m_SceneData.m_PointCloudShader->SetUniform("projection", &projectionMatrix, sizeof(glm::mat4));
		m_SceneData.m_PointCloudShader->SetUniform("pointSize", &pointSize, sizeof(float));

		if (m_SceneData.m_PointCloudShader->UpdateGlobalState())
		{
			for (auto [e, tr, mc, matc, mr] : group.each())
			{
				if (!mr.m_Rendered || !mc.HasMesh()) continue;

				Mesh& mesh = mc.GetMesh();
				if (!mesh.IsCloudPoint()) continue;
				if (mesh.HasSkinning())    continue;

				glm::mat4 model = tr.GetGlobalTransform();
				if (mc.HasLocalNodeTransform())
					model *= mc.GetLocalNodeTransform();

				m_SceneData.m_PointCloudShader->SetUniform("model", &model, sizeof(glm::mat4));

				//m_SceneData.m_PointCloudShader->SetStorageBuffer("vertices_buffer", mesh.GetVertices().data(), sizeof(float)* mesh.GetVertices().size());

				if (!m_SceneData.m_PointCloudShader->UpdateObject(nullptr))
					continue;

				mesh.draw();
			}
		}

		m_SceneData.m_PointCloudShader->Unuse();*/

		{
			Renderer2D::Instance().BeginScene(camera);

			for (auto e : m_SceneData.m_Scene->GetAllEntitiesWith<TransformComponent, SpriteComponent>())
			{
				Entity entity{ e, m_SceneData.m_Scene->GetRegistry() };

				auto& tr = entity.GetComponent<TransformComponent>();
				auto& sc = entity.GetComponent<SpriteComponent>();
				const auto& spec = sc.GetSpecification();
				if (!spec.Visible) continue;

				Texture* tex = sc.GetTexture();
				glm::mat4 T = tr.GetGlobalTransform();
				glm::vec4 uv = sc.GetEffectiveUV();

				Renderer2D::Instance().DrawQuad(
					T, tex, spec.Color, uv, spec.Tiling, spec.Offset, spec.SortingOrder
				);
			}

			Renderer2D::Instance().EndScene();
		}
	}

	void Renderer::RenderDebug(BaseCamera& camera)
	{
		/*if (PhysicEngine::Instance().GetDebugVertexArray())
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

			RenderCommand::Instance().DrawArrays(DrawMode::LINES, static_cast<uint32_t>(PhysicEngine::Instance().GetDebugVertexArray()->GetVertexBuffers()[0]->GetSize() / sizeof(float) / 6));

			m_SceneData.m_PhysicDebugShader->Unuse();
		}*/
	}

	void Renderer::RenderSkybox(BaseCamera& camera)
	{
		const glm::mat4 V = camera.getViewMatrix();
		const glm::mat4 P = camera.getProjectionMatrix();

		m_SceneData.m_SkyboxHDR->Draw(V, P);
	}

	void Renderer::RenderUI(BaseCamera& camera, int fbW, int fbH, float dpi)
	{
		UIFBInfo fb{ fbW, fbH, dpi };
		m_SceneData.m_UI->Render(camera, fb);
	}

	void Renderer::EndScene()
	{

	}

	void Renderer::BuildLight(BaseCamera& camera)
	{
		m_SceneData.nDirs = 0;
		m_SceneData.nPts = 0;

		for (auto e : m_SceneData.m_Scene->GetAllEntitiesWith<LightComponent, TransformComponent>())
		{
			Entity entity{ e, m_SceneData.m_Scene->GetRegistry() };

			auto& lc = entity.GetComponent<LightComponent>();
			auto& tr = entity.GetComponent<TransformComponent>();

			if (lc.lightType == LightComponent::LightType::DIRECTIONAL && m_SceneData.nDirs < 4)
			{
				glm::vec3 raysDir = Math::ForwardFromEulerRad(tr.Rotation);

				DirectionalLight dl = lc.directional_light;
				dl.direction = -raysDir;

				m_SceneData.m_DirectionalsBuffer[m_SceneData.nDirs++] = dl;
			}
			else if (lc.lightType == LightComponent::LightType::POINT && m_SceneData.nPts < 4)
			{
				PointLight pl = lc.point_light;
				pl.position = tr.Position;
				m_SceneData.m_PointsBuffer[m_SceneData.nPts++] = pl;
			}
		}
	}

	double Renderer::GetTime()
	{
		return glfwGetTime();
	}
}
