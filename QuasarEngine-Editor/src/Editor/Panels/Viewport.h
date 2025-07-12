#pragma once

#include <memory>

#include "../SceneObject.h"

#include <QuasarEngine/Renderer/Framebuffer.h>

#include <imgui/imgui.h>

namespace QuasarEngine
{
	class Viewport
	{
	public:
		Viewport();

		void Render(Scene& scene);
		void Update(Scene& scene);

		void OnImGuiRender(Scene& scene);
	private:
		std::shared_ptr<Framebuffer> m_ViewportFrameBuffer;

		glm::vec2 m_EditorViewportSize = { 0.0f, 0.0f };

		ImVec2 m_ViewportPanelSize = { 0, 0 };

		bool m_ViewportFocused = false;
		bool m_ViewportHovered = false;
	};
}