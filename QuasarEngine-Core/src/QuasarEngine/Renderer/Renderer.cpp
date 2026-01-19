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
#include <QuasarEngine/UI/UIButton.h>
#include <QuasarEngine/UI/UISeparator.h>
#include <QuasarEngine/UI/UIText.h>
#include <QuasarEngine/UI/UIContainer.h>
#include <QuasarEngine/UI/UICheckbox.h>
#include <QuasarEngine/UI/UISlider.h>
#include <QuasarEngine/UI/UIProgressBar.h>
#include <QuasarEngine/UI/UITextInput.h>
#include <QuasarEngine/UI/UITabBar.h>
#include <QuasarEngine/UI/UITooltipLayer.h>
#include <QuasarEngine/UI/UIMenu.h>

#include <QuasarEngine/Renderer/RenderCommand.h>
#include <QuasarEngine/Renderer/RendererAPI.h>
#include <QuasarEngine/Physic/PhysicEngine.h>
#include <limits>

#include <QuasarEngine/Renderer/RenderContext.h>
#include <QuasarEngine/Renderer/RenderObject.h>

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

		m_SceneData.m_StaticTech = std::make_unique<PBRStaticTechnique>(m_SceneData.m_SkyboxHDR.get());
		m_SceneData.m_SkinnedTech = std::make_unique<PBRSkinTechnique>(m_SceneData.m_SkyboxHDR.get());
		m_SceneData.m_TerrainTech = std::make_unique<TerrainTechnique>(m_SceneData.m_SkyboxHDR.get());
		//m_PcTech = std::make_unique<PointCloudTechnique>(m_SceneData.m_SkyboxHDR.get());

		m_SceneData.m_ScriptSystem = std::make_unique<ScriptSystem>();
		m_SceneData.m_ScriptSystem->Initialize();

		m_SceneData.m_PointsBuffer.fill(PointLight());
		m_SceneData.m_DirectionalsBuffer.fill(DirectionalLight());

		//m_SceneData.m_UI = std::make_unique<UISystem>();

		/*struct UISharedState {
			int quality = 1;
		};
		auto state = std::make_shared<UISharedState>();

		auto root = std::make_shared<UIContainer>("Root");
		root->layout = UILayoutType::Vertical;
		root->gap = 10.f;
		root->Style().padding = 12.f;
		root->Style().bg = { 0.10f, 0.10f, 0.12f, 0.85f };
		root->Transform().pos = { 10.f, 10.f };
		root->Transform().size = { 720.f, 0.f };

		{
			auto title = std::make_shared<UIText>("Title");
			title->text = "UI Playground - QuasarEngine";
			title->Style().bg = { 0,0,0,0 };
			title->Style().fg = { 0.95f, 0.96f, 0.99f, 1 };
			title->Transform().size = { 0.f, 36.f };
			root->AddChild(title);
		}

		auto row1 = std::make_shared<UIContainer>("Row1");
		row1->layout = UILayoutType::Horizontal;
		row1->gap = 12.f;
		row1->Style().bg = { 0,0,0,0 };
		root->AddChild(row1);

		auto colLeft = std::make_shared<UIContainer>("ColLeft");
		colLeft->layout = UILayoutType::Vertical;
		colLeft->gap = 6.f;
		colLeft->Style().padding = 8.f;
		colLeft->Style().bg = { 0.12f,0.13f,0.16f,1.f };
		colLeft->Transform().size = { 340.f, 0.f };
		row1->AddChild(colLeft);

		{
			auto btnResume = std::make_shared<UIButton>("BtnResume");
			btnResume->label = "Reprendre";
			btnResume->SetTabIndex(0);
			btnResume->onClick = []() { std::cout << "[UI] Resume clicked\n"; };
			colLeft->AddChild(btnResume);

			auto btnOptions = std::make_shared<UIButton>("BtnOptions");
			btnOptions->label = "Options";
			btnOptions->SetTabIndex(1);
			btnOptions->onClick = []() { std::cout << "[UI] Options clicked\n"; };
			colLeft->AddChild(btnOptions);

			auto btnQuit = std::make_shared<UIButton>("BtnQuit");
			btnQuit->label = "Quitter";
			btnQuit->SetTabIndex(2);
			btnQuit->onClick = []() { std::cout << "[UI] Quit clicked\n"; };
			colLeft->AddChild(btnQuit);
		}

		{
			auto cbVsync = std::make_shared<UICheckbox>("CbVsync");
			cbVsync->label = "VSync";
			cbVsync->SetTabIndex(3);
			colLeft->AddChild(cbVsync);

			auto cbPost = std::make_shared<UICheckbox>("CbPostFX");
			cbPost->label = "Post-Processing";
			cbPost->SetTabIndex(4);
			colLeft->AddChild(cbPost);

			auto cbDisabled = std::make_shared<UICheckbox>("CbDisableResume");
			cbDisabled->label = "Desactiver le bouton Reprendre (visuel)";
			cbDisabled->SetTabIndex(5);

			struct SyncDisable : UIElement {
				UIButton* target; UICheckbox* source;
				explicit SyncDisable(std::string id, UIButton* t, UICheckbox* s) : UIElement(std::move(id)), target(t), source(s) {}
				void Measure(UILayoutContext&) override { Transform().size = { 0,0 }; }
				void BuildDraw(UIRenderContext&) override {
					if (target && source) target->SetEnabled(!source->checked);
				}
			};

			colLeft->AddChild(cbDisabled);
			colLeft->AddChild(std::make_shared<SyncDisable>("SyncDisableResume",
				static_cast<UIButton*>(colLeft->Children()[0].get()),
				cbDisabled.get()));
		}

		{
			auto rLow = std::make_shared<UIRadioButton>("RadioLow");
			rLow->label = "Qualite : Basse";
			rLow->groupValue = &state->quality; rLow->index = 0;
			rLow->SetTabIndex(6);
			colLeft->AddChild(rLow);

			auto rMed = std::make_shared<UIRadioButton>("RadioMed");
			rMed->label = "Qualite : Moyenne";
			rMed->groupValue = &state->quality; rMed->index = 1;
			rMed->SetTabIndex(7);
			colLeft->AddChild(rMed);

			auto rHigh = std::make_shared<UIRadioButton>("RadioHigh");
			rHigh->label = "Qualite : Haute";
			rHigh->groupValue = &state->quality; rHigh->index = 2;
			rHigh->SetTabIndex(8);
			colLeft->AddChild(rHigh);
		}

		auto colRight = std::make_shared<UIContainer>("ColRight");
		colRight->layout = UILayoutType::Vertical;
		colRight->gap = 6.f;
		colRight->Style().padding = 8.f;
		colRight->Style().bg = { 0.12f,0.13f,0.16f,1.f };
		colRight->Transform().size = { 340.f, 0.f };
		row1->AddChild(colRight);

		std::shared_ptr<UISlider> sMaster, sSpeed;
		{
			auto lblVol = std::make_shared<UIText>("LblVolume"); lblVol->text = "Volume principal";
			colRight->AddChild(lblVol);

			sMaster = std::make_shared<UISlider>("SliderMaster");
			sMaster->min = 0; sMaster->max = 100; sMaster->value = 25;
			sMaster->SetTabIndex(9);
			colRight->AddChild(sMaster);

			auto lblSpeed = std::make_shared<UIText>("LblSpeed"); lblSpeed->text = "Vitesse";
			colRight->AddChild(lblSpeed);

			sSpeed = std::make_shared<UISlider>("SliderSpeed");
			sSpeed->min = 0; sSpeed->max = 10; sSpeed->value = 3.5f;
			sSpeed->SetTabIndex(10);
			colRight->AddChild(sSpeed);
		}

		{
			auto pb = std::make_shared<UIProgressBar>("ProgressLoad");
			pb->value = 0.25f;
			colRight->AddChild(pb);

			struct SyncProgress : UIElement {
				UIProgressBar* bar; UISlider* src;
				explicit SyncProgress(std::string id, UIProgressBar* b, UISlider* s) : UIElement(std::move(id)), bar(b), src(s) {}
				void Measure(UILayoutContext&) override { Transform().size = { 0,0 }; }
				void BuildDraw(UIRenderContext&) override {
					if (bar && src) bar->value = (src->value - src->min) / std::max(0.0001f, (src->max - src->min));
				}
			};
			colRight->AddChild(std::make_shared<SyncProgress>("SyncPB", pb.get(), sMaster.get()));
		}

		{
			auto lblName = std::make_shared<UIText>("LblName"); lblName->text = "Nom du projet";
			colRight->AddChild(lblName);

			auto inpName = std::make_shared<UITextInput>("InpName");
			inpName->text = "QuasarGame";
			inpName->SetTabIndex(11);
			colRight->AddChild(inpName);

			auto lblRO = std::make_shared<UIText>("LblRO"); lblRO->text = "Champ en lecture seule (disabled):";
			colRight->AddChild(lblRO);

			auto inpRO = std::make_shared<UITextInput>("InpRO");
			inpRO->text = "Indisponible";
			inpRO->SetEnabled(false);
			colRight->AddChild(inpRO);
		}

		auto tabs = std::make_shared<UITabs>("Tabs");
		tabs->Style().bg = { 0.11f,0.11f,0.13f,1 };
		tabs->Transform().size = { 0.f, 260.f };
		tabs->tabbar->labels = { "General", "Audio", "À propos" };
		root->AddChild(tabs);

		{
			auto gen = std::make_shared<UIContainer>("TabGeneral");
			gen->layout = UILayoutType::Vertical; gen->gap = 6.f;
			{
				auto t = std::make_shared<UIText>("TGen"); t->text = "Parametres generaux";
				gen->AddChild(t);

				auto cb = std::make_shared<UICheckbox>("GenCB"); cb->label = "Activer HUD";
				gen->AddChild(cb);
				auto cb2 = std::make_shared<UICheckbox>("GenCB2"); cb2->label = "Afficher FPS";
				gen->AddChild(cb2);
			}
			tabs->AddChild(gen);
		}

		{
			auto aud = std::make_shared<UIContainer>("TabAudio");
			aud->layout = UILayoutType::Vertical; aud->gap = 6.f;
			{
				auto t = std::make_shared<UIText>("TAud"); t->text = "Options audio";
				aud->AddChild(t);

				auto sfx = std::make_shared<UISlider>("SFX"); sfx->min = 0; sfx->max = 100; sfx->value = 70;
				aud->AddChild(sfx);
				auto mus = std::make_shared<UISlider>("Music"); mus->min = 0; mus->max = 100; mus->value = 40;
				aud->AddChild(mus);
			}
			tabs->AddChild(aud);
		}

		{
			auto about = std::make_shared<UIContainer>("TabAbout");
			about->layout = UILayoutType::Vertical; about->gap = 6.f;
			{
				auto t1 = std::make_shared<UIText>("Tab1"); t1->text = "QuasarEngine UI Sandbox";
				about->AddChild(t1);
				auto t2 = std::make_shared<UIText>("Tab2"); t2->text = "Test de widgets, layout, focus/tab, disabled, etc.";
				about->AddChild(t2);
			}
			tabs->AddChild(about);
		}

		auto menu = std::make_shared<UIMenu>("CtxMenu");
		menu->items = {
			{ "Nouveau",  false, []() { std::cout << "[Menu] Nouveau\n"; } },
			{ "Ouvrir...",false, []() { std::cout << "[Menu] Ouvrir\n"; } },
			{ "Enregistrer",false, []() { std::cout << "[Menu] Enregistrer\n"; } },
			{ "Quitter",  false, []() { std::cout << "[Menu] Quitter\n"; } }
		};
		root->AddChild(menu);

		{
			auto openMenuBtn = std::make_shared<UIButton>("BtnOpenMenu");
			openMenuBtn->label = "Ouvrir le menu";
			openMenuBtn->SetTabIndex(12);
			openMenuBtn->onClick = [menu, openMenuBtn]() {
				const Rect r = openMenuBtn->Transform().rect;
				menu->OpenAt(r.x, r.y + r.h + 4.f);
				};
			root->AddChild(openMenuBtn);
		}

		auto tooltip = std::make_shared<UITooltipLayer>("Tooltip");
		root->AddChild(tooltip);

		tooltip->Show("Astuce: Tab pour naviguer, Espace/Entree pour activer.", 28.f, 80.f);

		m_SceneData.m_UI->SetRoot(root);*/

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

		//m_SceneData.m_SmokeEmitter = std::make_unique<SmokeEmitterScript>();
	}

	void Renderer::Shutdown()
	{
		m_SceneData.m_SkyboxHDR.reset();
		//m_SceneData.m_PhysicDebugShader.reset();
		//m_SceneData.m_PointCloudShader.reset();
		//m_SceneData.m_UI.reset();
		m_SceneData.m_ScriptSystem.reset();
		//m_SceneData.m_SmokeEmitter.reset();
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

		if (!staticMeshes.empty())
		{
			m_SceneData.m_StaticTech->Begin(ctx);
			for (auto& obj : staticMeshes)
			{
				m_SceneData.m_StaticTech->Submit(ctx, obj);
			}
			m_SceneData.m_StaticTech->End();
		}

		if (!skinnedMeshes.empty())
		{
			m_SceneData.m_SkinnedTech->Begin(ctx);
			for (auto& obj : skinnedMeshes)
			{
				m_SceneData.m_SkinnedTech->Submit(ctx, obj);
			}
			m_SceneData.m_SkinnedTech->End();
		}

		if (!terrains.empty())
		{
			m_SceneData.m_TerrainTech->Begin(ctx);
			for (auto& obj : terrains)
				m_SceneData.m_TerrainTech->Submit(ctx, obj);
			m_SceneData.m_TerrainTech->End();
		}

		for (auto [e, tr, pc] : m_SceneData.m_Scene->GetRegistry()->GetRegistry().group<TransformComponent, ParticleComponent>().each())
		{
			pc.Render(ctx);
		}

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

		/* {
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
		}*/
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
		UIFBInfo fb{fbW, fbH, dpi};
		//m_SceneData.m_UI->Render(camera, fb);
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
