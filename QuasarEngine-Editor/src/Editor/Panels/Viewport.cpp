#include "Viewport.h"

#include <QuasarEngine/Core/Application.h>
#include <QuasarEngine/Renderer/RenderCommand.h>
#include <QuasarEngine/Renderer/RendererAPI.h>

#include <QuasarEngine/Core/Input.h>

namespace QuasarEngine
{
	Viewport::Viewport()
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

		RenderCommand::ClearColor(m_ClearColor);
		RenderCommand::Clear();

		if (scene.HasPrimaryCamera())
		{
			Renderer::BeginScene(scene);

			Camera& camera = scene.GetPrimaryCamera();
			Renderer::RenderSkybox(camera);
			Renderer::Render(camera);
			Renderer::RenderUI(camera);

			Renderer::EndScene();
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
		const ImVec2 mouseImGui = ImGui::GetIO().MousePos;

		if (mouseImGui.x < m_ViewportBounds[0].x || mouseImGui.x > m_ViewportBounds[1].x ||
			mouseImGui.y < m_ViewportBounds[0].y || mouseImGui.y > m_ViewportBounds[1].y) {
			return false;
		}

		glm::vec2 local = {
			mouseImGui.x - m_ViewportBounds[0].x,
			mouseImGui.y - m_ViewportBounds[0].y
		};

		const auto fbW = (float)m_ViewportFrameBuffer->GetSpecification().Width;
		const auto fbH = (float)m_ViewportFrameBuffer->GetSpecification().Height;
		const float sx = fbW / m_ViewportPanelSize.x;
		const float sy = fbH / m_ViewportPanelSize.y;
		glm::vec2 uiPos = { local.x * sx, local.y * sy };

		UIPointerEvent ev;
		ev.x = uiPos.x;
		ev.y = uiPos.y;
		ev.down = true;
		ev.button = e.GetMouseButton();

		Renderer::m_SceneData.m_UI->Input().FeedPointer(ev);

		//std::cout << "Mouse button pressed: " << e.GetMouseButton() << " at (" << ev.x << ", " << ev.y << ")\n";

		return true;
	}


	bool Viewport::OnMouseButtonReleased(MouseButtonReleasedEvent& e)
	{
		glm::vec2 mousePos = Input::GetMousePosition() - m_ViewportBounds[0];
		UIPointerEvent ev; ev.x = mousePos.x; ev.y = mousePos.y; ev.up = true; ev.button = e.GetMouseButton();
		Renderer::m_SceneData.m_UI->Input().FeedPointer(ev);

		return false;
	}

	bool Viewport::OnMouseMoved(MouseMovedEvent& e)
	{
		UIPointerEvent ev; ev.x = e.GetX(); ev.y = e.GetY(); ev.move = true;
		Renderer::m_SceneData.m_UI->Input().FeedPointer(ev);

		return false;
	}
}