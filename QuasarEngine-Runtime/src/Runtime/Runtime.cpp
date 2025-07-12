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

#include <QuasarEngine/Entity/Entity.h>
#include <QuasarEngine/Tools/Chronometer.h>
#include <QuasarEngine/Entity/Components/MeshComponent.h>

namespace QuasarEngine
{
	Runtime::Runtime() : Layer("Runtime")
	{
		Application::Get().MaximizeWindow(true);
	}

	void Runtime::OnAttach()
	{
		//RenderCommand::Init();

		//PhysicEngine::Init();
		//Renderer::Init();

		Application::Get().GetWindow().SetCursorVisibility(true);

		//m_ScreenQuad = std::make_unique<ScreenQuad>();
		m_Scene = std::make_unique<Scene>();

		Renderer::BeginScene(*m_Scene);

		//ShaderFile shaderfile;
		//shaderfile.vertexShaderFile = "Assets/Shaders/ScreenQuad.vert";
		//shaderfile.fragmentShaderFile = "Assets/Shaders/ScreenQuad.frag";
		//m_ScreenQuadShader = Shader::Create(shaderfile);
		//m_ScreenQuadShader->setUniform("screenTexture", 0);

		//FramebufferSpecification spec;
		//spec.Width = Application::Get().GetWindow().GetWidth();
		//spec.Height = Application::Get().GetWindow().GetHeight();
		//spec.Attachments = { FramebufferTextureFormat::RGBA8, FramebufferTextureFormat::Depth };

		//m_FrameBuffer = Framebuffer::Create(spec);
		//m_ApplicationSize = { spec.Width, spec.Height };
		m_ApplicationSize = { 1280, 720 };
		//RenderCommand::SetViewport(0, 0, m_ApplicationSize.x, m_ApplicationSize.y);
		//m_FrameBuffer->Resize((uint32_t)m_ApplicationSize.x, (uint32_t)m_ApplicationSize.y);

		m_ChunkManager = std::make_unique<ChunkManager>();
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

		/*Entity player_light = m_Scene->CreateEntity("PlayerLight");
		auto& player_light_component = player_light.AddComponent<LightComponent>();
		player_light_component.SetType(QuasarEngine::LightComponent::LightType::POINT);
		player_light_component.point_power = 60.0f;
		player_light_component.point_attenuation = 0.2f;*/
	}

	void Runtime::OnDetach()
	{
		//PhysicEngine::Shutdown();
	}

	void Runtime::OnUpdate(double dt)
	{
		m_Player->Update(dt);

		m_Player->GetCamera().Update();

		m_Scene->Update(dt);

		m_ChunkManager->UpdateChunk(m_Player->GetPosition(), dt);
	}

	void Runtime::OnRender()
	{
		//m_FrameBuffer->Bind();

		//RenderCommand::Clear();
		//RenderCommand::ClearColor(glm::vec4(0.1f, 0.5f, .9f, 1.0f));

		//Renderer::BeginScene(*m_Scene);
		
		//Renderer::RenderSkybox(m_Player->GetCamera());
		//Renderer::EndScene();

		unsigned int width = Application::Get().GetWindow().GetWidth();
		unsigned int height = Application::Get().GetWindow().GetHeight();
		if (m_ApplicationSize.x != width || m_ApplicationSize.y != height)
		{
			//RenderCommand::SetViewport(0, 0, width, height);
			m_Player->GetCamera().OnResize(width, height);
			//m_FrameBuffer->Resize((uint32_t)width, (uint32_t)height);

			m_ApplicationSize = { width, height };
		}

		//m_FrameBuffer->Unbind();

		//m_ScreenQuadShader->Use();
		//m_FrameBuffer->BindColorAttachment(0);
		//m_ScreenQuad->Draw();
	}

	void Runtime::OnGuiRender()
	{
		Renderer::BeginScene(*m_Scene);
		Renderer::Render(m_Player->GetCamera());
		Renderer::EndScene();
	}

	void Runtime::OnEvent(Event& e)
	{
		m_Player->GetCamera().OnEvent(e);
	}
}
