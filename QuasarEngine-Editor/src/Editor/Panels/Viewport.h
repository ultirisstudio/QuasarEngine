#pragma once

#include <memory>
#include <glm/glm.hpp>
#include <imgui/imgui.h>

#include "../SceneObject.h"
#include <QuasarEngine/Renderer/Framebuffer.h>
#include <QuasarEngine/Renderer/Renderer.h>

namespace QuasarEngine
{
	class Viewport
	{
	public:
		Viewport();

		void Render(Scene& scene);

		void Update(Scene& scene);

		void OnImGuiRender(Scene& scene);

		const glm::vec2& GetViewportSize() const { return m_ViewportSize; }
		bool IsFocused() const { return m_ViewportFocused; }
		bool IsHovered() const { return m_ViewportHovered; }
		void SetClearColor(const glm::vec4& color) { m_ClearColor = color; }

	private:
		void ResizeIfNeeded(Scene& scene, const ImVec2& panelSize);

	private:
		std::shared_ptr<Framebuffer> m_ViewportFrameBuffer;

		glm::vec2 m_ViewportSize = { 0.0f, 0.0f };
		glm::vec2 m_ViewportBounds[2];
		ImVec2    m_ViewportPanelSize = { 0, 0 };

		glm::vec4 m_ClearColor = { 0.2f, 0.2f, 0.2f, 1.0f };

		bool m_ViewportFocused = false;
		bool m_ViewportHovered = false;
	};
}
