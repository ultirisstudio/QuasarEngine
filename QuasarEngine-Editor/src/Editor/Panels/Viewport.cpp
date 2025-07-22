#include "Viewport.h"

#include <QuasarEngine/Core/Application.h>

QuasarEngine::Viewport::Viewport()
{
	FramebufferSpecification spec;
	spec.Width = Application::Get().GetWindow().GetWidth();
	spec.Height = Application::Get().GetWindow().GetHeight();
	spec.Attachments = { FramebufferTextureFormat::RGBA8, FramebufferTextureFormat::Depth };

	m_ViewportFrameBuffer = Framebuffer::Create(spec);
	m_ViewportFrameBuffer->Invalidate();
}

void QuasarEngine::Viewport::Render(Scene& scene)
{
	if (!scene.HasPrimaryCamera())
		return;

	m_ViewportFrameBuffer->Bind();

	Renderer::BeginScene(scene);

	Renderer::Render(scene.GetPrimaryCamera());
	Renderer::EndScene();

	m_ViewportFrameBuffer->Unbind();
}

void QuasarEngine::Viewport::Update(Scene& scene)
{
	if (m_EditorViewportSize != *((glm::vec2*)&m_ViewportPanelSize))
	{
		scene.GetPrimaryCamera().OnResize(m_ViewportPanelSize.x, m_ViewportPanelSize.y);
		m_ViewportFrameBuffer->Resize((uint32_t)m_ViewportPanelSize.x, (uint32_t)m_ViewportPanelSize.y);
		m_EditorViewportSize = { m_ViewportPanelSize.x, m_ViewportPanelSize.y };
	}
}

void QuasarEngine::Viewport::OnImGuiRender(Scene& scene)
{
	ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2{ 0, 0 });
	ImGui::Begin("Viewport");

	if (scene.HasPrimaryCamera())
	{
		m_ViewportFocused = ImGui::IsWindowFocused();
		m_ViewportHovered = ImGui::IsWindowHovered();

		m_ViewportPanelSize = ImGui::GetContentRegionAvail();

		if (m_ViewportFrameBuffer)
		{
			void* handle = m_ViewportFrameBuffer->GetColorAttachment(0);
			if (handle)
			{
				ImGui::Image((ImTextureID)handle, ImVec2{ m_EditorViewportSize.x, m_EditorViewportSize.y }, ImVec2{ 0, 0 }, ImVec2{ 1, 1 });
			}
		}
	}

	ImGui::End();
	ImGui::PopStyleVar();
}
