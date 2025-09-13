#include "Viewport.h"

#include <QuasarEngine/Core/Application.h>
#include <QuasarEngine/Renderer/RenderCommand.h>
#include <QuasarEngine/Renderer/RendererAPI.h>

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

	void Viewport::Update(Scene& scene)
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

		ImVec2 viewportMin = ImGui::GetCursorScreenPos();
		ImVec2 viewportSize = ImGui::GetContentRegionAvail();
		m_ViewportPanelSize = viewportSize;

		ResizeIfNeeded(scene, viewportSize);

		m_ViewportBounds[0] = { viewportMin.x,                      viewportMin.y };
		m_ViewportBounds[1] = { viewportMin.x + viewportSize.x,     viewportMin.y + viewportSize.y };

		if (m_ViewportFrameBuffer)
		{
			if (void* handle = m_ViewportFrameBuffer->GetColorAttachment(0))
			{
				ImVec2 uv0 = (RendererAPI::GetAPI() == RendererAPI::API::OpenGL) ? ImVec2{ 0, 1 } : ImVec2{ 0, 0 };
				ImVec2 uv1 = (RendererAPI::GetAPI() == RendererAPI::API::OpenGL) ? ImVec2{ 1, 0 } : ImVec2{ 1, 1 };

				ImGui::Image((ImTextureID)handle, viewportSize, uv0, uv1);
			}
		}

		ImGui::End();
		ImGui::PopStyleVar();
	}
}