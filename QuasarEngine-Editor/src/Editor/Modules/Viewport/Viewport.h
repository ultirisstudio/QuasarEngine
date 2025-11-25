#pragma once

#include <memory>
#include <glm/glm.hpp>
#include <imgui/imgui.h>

#include <QuasarEngine/Renderer/Framebuffer.h>
#include <QuasarEngine/Renderer/Renderer.h>

#include <QuasarEngine/Events/Event.h>
#include <QuasarEngine/Events/MouseEvent.h>
#include <QuasarEngine/Events/KeyEvent.h>

#include <Editor/Modules/IEditorModule.h>

namespace QuasarEngine
{
	class Viewport : public IEditorModule
	{
	public:
		Viewport(EditorContext& context);
		~Viewport() override;

		void Update(double dt) override;
		void Render() override;
		void RenderUI() override;

		void OnEvent(Event& e) override;

		const glm::vec2& GetViewportSize() const { return m_ViewportSize; }
		bool IsFocused() const { return m_ViewportFocused; }
		bool IsHovered() const { return m_ViewportHovered; }
		void SetClearColor(const glm::vec4& color) { m_ClearColor = color; }

	private:
		void ResizeIfNeeded(const ImVec2& panelSize);

		bool OnMouseButtonPressed(MouseButtonPressedEvent& e);
		bool OnMouseButtonReleased(MouseButtonReleasedEvent& e);
		bool OnMouseMoved(MouseMovedEvent& e);
		bool OnKeyPressed(KeyPressedEvent& e);
		bool OnKeyReleased(KeyReleasedEvent& e);
		bool OnKeyTyped(KeyTypedEvent& e);

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
