#include "Viewport.h"

#include <QuasarEngine/Core/Application.h>
#include <QuasarEngine/Renderer/RenderCommand.h>
#include <QuasarEngine/Renderer/RendererAPI.h>

#include <QuasarEngine/UI/UIDebugOverlay.h>

#include <QuasarEngine/Core/Input.h>

namespace QuasarEngine
{
	Viewport::Viewport() : m_ViewportBounds{ {0,0}, {0,0} }, m_ViewportSize({ 0.0f, 0.0f }), m_ViewportPanelSize({ 0.0f, 0.0f }), m_ClearColor({ 0.2f, 0.2f, 0.2f, 1.0f })
	{
		FramebufferSpecification spec;
		spec.Width = Application::Get().GetWindow().GetWidth();
		spec.Height = Application::Get().GetWindow().GetHeight();
		spec.Attachments = {
			FramebufferTextureFormat::RGBA8,
			FramebufferTextureFormat::Depth
		};

		m_ViewportFrameBuffer = Framebuffer::Create(spec);
		m_ViewportFrameBuffer->Invalidate();
	}

	void Viewport::Render(Scene& scene)
	{
		if (!m_ViewportFrameBuffer)
			return;

		m_ViewportFrameBuffer->Bind();

		const auto& spec = m_ViewportFrameBuffer->GetSpecification();
		RenderCommand::Instance().SetViewport(0, 0, spec.Width, spec.Height);

		RenderCommand::Instance().ClearColor(m_ClearColor);
		RenderCommand::Instance().Clear();

		if (scene.HasPrimaryCamera())
		{
			Renderer::Instance().BeginScene(scene);

			Camera& camera = scene.GetPrimaryCamera();
			const auto& spec = m_ViewportFrameBuffer->GetSpecification();
			const int fbW = (int)spec.Width;
			const int fbH = (int)spec.Height;

			float dpiScale = 1.0f;

			if (ImGui::GetMainViewport())
				dpiScale = ImGui::GetMainViewport()->DpiScale;
			else
				dpiScale = ImGui::GetIO().DisplayFramebufferScale.x;

			Renderer::Instance().RenderSkybox(camera);
			Renderer::Instance().Render(camera);
			Renderer::Instance().RenderUI(camera, fbW, fbH, dpiScale);

			Renderer::Instance().EndScene();
		}

		m_ViewportFrameBuffer->Unbind();
	}

	void Viewport::ResizeIfNeeded(Scene& scene, const ImVec2& panelSize)
	{
		const uint32_t w = (uint32_t)std::max(1.0f, panelSize.x);
		const uint32_t h = (uint32_t)std::max(1.0f, panelSize.y);

		if ((uint32_t)m_ViewportSize.x != w || (uint32_t)m_ViewportSize.y != h)
		{
			if (m_ViewportFrameBuffer)
				m_ViewportFrameBuffer->Resize(w, h);

			if (scene.HasPrimaryCamera())
				scene.GetPrimaryCamera().OnResize(w, h);

			m_ViewportSize = { (float)w, (float)h };
		}
	}

	void Viewport::Update(Scene& scene, double dt)
	{
		ResizeIfNeeded(scene, m_ViewportPanelSize);
	}

	void Viewport::OnImGuiRender(Scene& scene)
	{
		ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2{ 0, 0 });
		ImGuiWindowFlags vpFlags = ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse;
		ImGui::Begin("Viewport", nullptr, vpFlags);

		m_ViewportFocused = ImGui::IsWindowFocused(ImGuiFocusedFlags_RootAndChildWindows);
		m_ViewportHovered = ImGui::IsWindowHovered(ImGuiHoveredFlags_RootAndChildWindows);

		const ImVec2 winPos = ImGui::GetWindowPos();
		const ImVec2 crMin = ImGui::GetWindowContentRegionMin();
		const ImVec2 crMax = ImGui::GetWindowContentRegionMax();
		const ImVec2 vpMin = ImVec2{ winPos.x + crMin.x, winPos.y + crMin.y };
		const ImVec2 vpMax = ImVec2{ winPos.x + crMax.x, winPos.y + crMax.y };
		const ImVec2 vpSize = ImVec2{ vpMax.x - vpMin.x, vpMax.y - vpMin.y };

		m_ViewportPanelSize = vpSize;
		m_ViewportBounds[0] = { vpMin.x, vpMin.y };
		m_ViewportBounds[1] = { vpMax.x, vpMax.y };

		//ResizeIfNeeded(scene, vpSize);

		if (m_ViewportFrameBuffer) {
			if (void* handle = m_ViewportFrameBuffer->GetColorAttachment(0)) {
				ImVec2 uv0 = (RendererAPI::GetAPI() == RendererAPI::API::OpenGL) ? ImVec2{ 0,1 } : ImVec2{ 1,0 };
				ImVec2 uv1 = (RendererAPI::GetAPI() == RendererAPI::API::OpenGL) ? ImVec2{ 1,0 } : ImVec2{ 0,1 };
				ImGui::SetCursorScreenPos(vpMin);
				ImGui::Image((ImTextureID)handle, vpSize, uv0, uv1);
			}
		}

		QuasarEngine::UIDebugOverlay::Instance().DrawImGui(vpMin);

		ImGui::End();
		ImGui::PopStyleVar();
	}

	void Viewport::OnEvent(Event& e)
	{
		EventDispatcher dispatcher(e);
		dispatcher.Dispatch<MouseButtonPressedEvent>(std::bind(&Viewport::OnMouseButtonPressed, this, std::placeholders::_1));
		dispatcher.Dispatch<MouseButtonReleasedEvent>(std::bind(&Viewport::OnMouseButtonReleased, this, std::placeholders::_1));
		dispatcher.Dispatch<MouseMovedEvent>(std::bind(&Viewport::OnMouseMoved, this, std::placeholders::_1));
	}

	bool Viewport::OnMouseButtonPressed(MouseButtonPressedEvent& e)
	{
		const ImVec2 imguiPos = ImGui::GetIO().MousePos;
		const glm::vec2 uiPos = ToUi(imguiPos);

		const auto fbW = (float)m_ViewportFrameBuffer->GetSpecification().Width;
		const auto fbH = (float)m_ViewportFrameBuffer->GetSpecification().Height;

		if (uiPos.x < 0.0f || uiPos.y < 0.0f || uiPos.x >= fbW || uiPos.y >= fbH)
			return false;

		UIPointerEvent ev{};
		ev.x = uiPos.x;
		ev.y = uiPos.y;
		ev.down = true;
		ev.button = e.GetMouseButton();

		//Renderer::Instance().m_SceneData.m_UI->Input().FeedPointer(ev);
		return false;
	}

	bool Viewport::OnMouseButtonReleased(MouseButtonReleasedEvent& e)
	{
		const ImVec2 imguiPos = ImGui::GetIO().MousePos;
		const glm::vec2 uiPos = ToUi(imguiPos);

		const auto fbW = (float)m_ViewportFrameBuffer->GetSpecification().Width;
		const auto fbH = (float)m_ViewportFrameBuffer->GetSpecification().Height;

		if (uiPos.x < 0.0f || uiPos.y < 0.0f || uiPos.x >= fbW || uiPos.y >= fbH)
			return false;

		UIPointerEvent ev{};
		ev.x = uiPos.x;
		ev.y = uiPos.y;
		ev.up = true;
		ev.button = e.GetMouseButton();

		//Renderer::Instance().m_SceneData.m_UI->Input().FeedPointer(ev);
		return false;
	}

	bool Viewport::OnMouseMoved(MouseMovedEvent& /*e*/)
	{
		const ImVec2 imguiPos = ImGui::GetIO().MousePos;
		const glm::vec2 uiPos = ToUi(imguiPos);

		const auto fbW = (float)m_ViewportFrameBuffer->GetSpecification().Width;
		const auto fbH = (float)m_ViewportFrameBuffer->GetSpecification().Height;

		if (uiPos.x < 0.0f || uiPos.y < 0.0f || uiPos.x >= fbW || uiPos.y >= fbH)
			return false;

		UIPointerEvent ev{};
		ev.x = uiPos.x;
		ev.y = uiPos.y;
		ev.move = true;

		//Renderer::Instance().m_SceneData.m_UI->Input().FeedPointer(ev);
		return false;
	}

	glm::vec2 Viewport::ToUi(const ImVec2& imguiPos)
	{
		const float sx = (float)m_ViewportFrameBuffer->GetSpecification().Width / m_ViewportPanelSize.x;
		const float sy = (float)m_ViewportFrameBuffer->GetSpecification().Height / m_ViewportPanelSize.y;
		return { (imguiPos.x - m_ViewportBounds[0].x) * sx,
				 (imguiPos.y - m_ViewportBounds[0].y) * sy };
	}
}