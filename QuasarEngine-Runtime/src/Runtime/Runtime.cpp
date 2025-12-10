#include "Runtime.h"

#include <glm/glm.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <glm/gtx/matrix_decompose.hpp>

#include "imgui/imgui.h"
#include "imgui/imgui_internal.h"
#include "ImGuizmo.h"

#include <QuasarEngine/Physic/PhysicEngine.h>
#include <QuasarEngine/Entity/Components/CameraComponent.h>
#include <QuasarEngine/Entity/Components/LightComponent.h>
#include <QuasarEngine/Shader/Shader.h>
#include <QuasarEngine/Renderer/RendererAPI.h>
#include <QuasarEngine/Renderer/RenderCommand.h>
#include <QuasarEngine/Renderer/Renderer2D.h>
#include <QuasarEngine/Core/Logger.h>

#include <QuasarEngine/Entity/Entity.h>
#include <QuasarEngine/Tools/Chronometer.h>
#include <QuasarEngine/Entity/Components/MeshComponent.h>

namespace QuasarEngine
{
	Runtime::Runtime() : Layer("Runtime")
	{
		//Application::Get().MaximizeWindow(true);
	}

	void Runtime::OnAttach()
	{
		AssetManager::Instance().Initialize("");

		RenderCommand::Instance().Initialize();
		Renderer::Instance().Initialize();
		Renderer2D::Instance().Initialize();

		PhysicEngine::Instance().Initialize();

		Logger::initUtf8Console();

		Application::Get().GetWindow().SetCursorVisibility(true);

		m_ScreenQuad = std::make_unique<ScreenQuad>();

		//m_Scene = std::make_unique<Scene>();
		//Renderer::Instance().BeginScene(*m_Scene);
		
		m_SceneManager = std::make_unique<SceneManager>("");
		m_SceneManager->LoadScene("Assets/Scenes/script_test.scene");

		Shader::ShaderDescription screenDesc;

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


		Shader::ShaderDescription desc;

		const auto api = RendererAPI::GetAPI();
		const std::string basePath = (api == RendererAPI::API::Vulkan)
			? "Assets/Shaders/vk/spv/"
			: "Assets/Shaders/gl/";

		const std::string name = "ScreenQuad";

		std::string vertPath = basePath + name + extFor(api, Shader::ShaderStageType::Vertex);
		std::string fragPath = basePath + name + extFor(api, Shader::ShaderStageType::Fragment);

		screenDesc.modules = {
			Shader::ShaderModuleInfo{
				Shader::ShaderStageType::Vertex,
				vertPath,
				"",
				{
					{0, Shader::ShaderIOType::Vec2, "aPos", true, ""},
					{1, Shader::ShaderIOType::Vec2, "aTexCoords", true, ""}
				}
			},
			Shader::ShaderModuleInfo{
				Shader::ShaderStageType::Fragment,
				fragPath,
				"",
				{
					{0, Shader::ShaderIOType::Vec4, "FragColor", false, ""}
				}
			}
		};

		screenDesc.globalUniforms = {};

		screenDesc.objectUniforms = {};

		screenDesc.samplers = {
			{"screenTexture", 0, 0, Shader::StageToBit(Shader::ShaderStageType::Fragment)}
		};

		screenDesc.blendMode = Shader::BlendMode::None;
		screenDesc.cullMode = Shader::CullMode::None;
		screenDesc.fillMode = Shader::FillMode::Solid;
		screenDesc.depthFunc = Shader::DepthFunc::Always;
		screenDesc.depthTestEnable = false;
		screenDesc.depthWriteEnable = false;
		screenDesc.topology = Shader::PrimitiveTopology::TriangleList;
		screenDesc.enableDynamicViewport = true;
		screenDesc.enableDynamicScissor = true;

		m_ScreenQuadShader = Shader::Create(screenDesc);

		FramebufferSpecification spec;
		spec.Width = Application::Get().GetWindow().GetWidth();
		spec.Height = Application::Get().GetWindow().GetHeight();
		spec.Attachments = {
				FramebufferTextureFormat::RGBA8,
				FramebufferTextureFormat::Depth
		};
		spec.Samples = 1;

		m_FrameBuffer = Framebuffer::Create(spec);
		m_FrameBuffer->Invalidate();

		m_ApplicationSize = { spec.Width, spec.Height };

		RenderCommand::Instance().SetViewport(0u, 0u, static_cast<uint32_t>(m_ApplicationSize.x), static_cast<uint32_t>(m_ApplicationSize.y));
		m_FrameBuffer->Resize(static_cast<uint32_t>(m_ApplicationSize.x), static_cast<uint32_t>(m_ApplicationSize.y));

		/*m_ChunkManager = std::make_unique<ChunkManager>();
		m_Player = std::make_unique<Player>();

		m_Player->GetCamera().OnResize(m_ApplicationSize.x, m_ApplicationSize.y);

		std::cout << "Voxel: " << sizeof(Block) << std::endl;
		std::cout << "Chunk: " << sizeof(Chunk) << std::endl;
		std::cout << "ChunkManager: " << sizeof(ChunkManager) << std::endl;
		std::cout << "Mesh: " << sizeof(Mesh) << std::endl;
		std::cout << "VoxelType: " << sizeof(BlockType) << std::endl;

		Entity light = m_Scene->CreateEntity("Light");
		light.GetComponent<TransformComponent>().Rotation = { 20, 90, 45};
		auto& light_component = light.AddComponent<LightComponent>();
		light_component.SetType(QuasarEngine::LightComponent::LightType::DIRECTIONAL);
		light_component.directional_light.power = 30.0f;

		Entity player_light = m_Scene->CreateEntity("PlayerLight");
		auto& player_light_component = player_light.AddComponent<LightComponent>();
		player_light_component.SetType(QuasarEngine::LightComponent::LightType::POINT);
		player_light_component.point_light.power = 60.0f;
		player_light_component.point_light.attenuation = 0.2f;*/

		//Application::Get().GetWindow().SetCursorVisibility(false);
		//Application::Get().GetWindow().SetInputMode(true, false);
	}

	void Runtime::OnDetach()
	{
		PhysicEngine::Instance().Shutdown();
		AssetManager::Instance().Shutdown();
		Renderer2D::Instance().Shutdown();
		Renderer::Instance().Shutdown();
		RenderCommand::Instance().Shutdown();
	}

	void Runtime::OnUpdate(double dt)
	{
		Input::Update();

		/*m_Scene->Update(dt);

		m_Player->Update(dt);

		m_Player->GetCamera().Update();

		m_ChunkManager->UpdateChunks(glm::floor(m_Player->GetPosition()), dt);*/

		m_SceneManager->Update(dt);

		static bool test = false;
		if (test == false) {
			m_SceneManager->GetActiveScene().OnRuntimeStart();
			test = true;
		}
	}

	void Runtime::OnRender()
	{
		m_FrameBuffer->Bind();

		RenderCommand::Instance().ClearColor({ 0.8f, 0.8f, 0.8f, 1.0f });
		RenderCommand::Instance().Clear();

		const auto& spec = m_FrameBuffer->GetSpecification();
		RenderCommand::Instance().SetViewport(0, 0, spec.Width, spec.Height);
		RenderCommand::Instance().SetScissor(0, 0, spec.Width, spec.Height);

		Renderer::Instance().BeginScene(m_SceneManager->GetActiveScene());
		//Renderer::Instance().BeginScene(*m_Scene.get());

		//Renderer::Instance().CollectLights(*m_Scene.get());

		if (m_SceneManager->GetActiveScene().HasPrimaryCamera()) {

			Camera& camera = m_SceneManager->GetActiveScene().GetPrimaryCamera();

			Renderer::Instance().BuildLight(camera);

			Renderer::Instance().RenderSkybox(camera);
			Renderer::Instance().Render(camera);

			//Renderer::Instance().RenderSkybox(m_Player->GetCamera());
			//Renderer::Instance().Render(m_Player->GetCamera());

			Renderer::Instance().EndScene();
		}

		m_FrameBuffer->Unbind();

		m_FrameBuffer->Resolve();

		RenderCommand::Instance().SetViewport(0, 0, m_ApplicationSize.x, m_ApplicationSize.y);

		m_ScreenQuadShader->Use();

		m_ScreenQuadShader->UpdateGlobalState();

		auto screenTex = m_FrameBuffer->GetColorAttachmentTexture(0);
		m_ScreenQuadShader->SetTexture("screenTexture", screenTex.get());

		m_ScreenQuadShader->UpdateObject(nullptr);

		m_ScreenQuad->Draw();

		m_ScreenQuadShader->Unuse();
	}

	void Runtime::OnGuiRender()
	{
		/*ImGui::Begin("Runtime");

		ImGui::Image((ImTextureID)(uintptr_t)m_FrameBuffer->GetColorAttachmentTexture(0)->GetHandle(), ImVec2(512, 512), ImVec2(0, 1), ImVec2(1, 0));

		ImGui::End();*/
	}

	void Runtime::OnEvent(Event& e)
	{
		EventDispatcher dispatcher(e);
		dispatcher.Dispatch<WindowResizeEvent>(std::bind(&Runtime::OnWindowResize, this, std::placeholders::_1));
		 
		//m_Player->GetCamera().OnEvent(e);
	}

	bool Runtime::OnWindowResize(WindowResizeEvent& e)
	{
		//RenderCommand::Instance().SetViewport(0, 0, e.GetWidth(), e.GetHeight());

		m_FrameBuffer->Resize(e.GetWidth(), e.GetHeight());

		if (m_SceneManager->GetActiveScene().HasPrimaryCamera()) {
			Camera& camera = m_SceneManager->GetActiveScene().GetPrimaryCamera();
			camera.OnResize(e.GetWidth(), e.GetHeight());
		}

		//m_Player->GetCamera().OnResize(e.GetWidth(), e.GetHeight());

		m_ApplicationSize = { e.GetWidth(), e.GetHeight() };

		return false;
	}
}
