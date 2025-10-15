#pragma once

#include <memory>
#include <glm/glm.hpp>
#include <imgui/imgui.h>

#include <QuasarEngine/Renderer/Framebuffer.h>
#include <QuasarEngine/Renderer/Renderer.h>

#include <QuasarEngine/Events/Event.h>
#include <QuasarEngine/Events/MouseEvent.h>

namespace QuasarEngine
{
	class Viewport
	{
	public:
		Viewport();

		void Render(Scene& scene);

		void Update(Scene& scene, double dt);

		void OnImGuiRender(Scene& scene);

		void OnEvent(Event& e);

		const glm::vec2& GetViewportSize() const { return m_ViewportSize; }
		bool IsFocused() const { return m_ViewportFocused; }
		bool IsHovered() const { return m_ViewportHovered; }
		void SetClearColor(const glm::vec4& color) { m_ClearColor = color; }

	private:
		void ResizeIfNeeded(Scene& scene, const ImVec2& panelSize);

		bool OnMouseButtonPressed(MouseButtonPressedEvent& e);
		bool OnMouseButtonReleased(MouseButtonReleasedEvent& e);
		bool OnMouseMoved(MouseMovedEvent& e);

		glm::vec2 ToUi(const ImVec2& imguiPos);

	private:
		std::shared_ptr<Framebuffer> m_ViewportFrameBuffer;

		glm::vec2 m_ViewportSize;
		glm::vec2 m_ViewportBounds[2];
		ImVec2    m_ViewportPanelSize;

		glm::vec4 m_ClearColor;

		bool m_ViewportFocused = false;
		bool m_ViewportHovered = false;
	};
}
