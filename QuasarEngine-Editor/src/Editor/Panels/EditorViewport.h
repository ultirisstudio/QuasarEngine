#pragma once

#include <memory>

#include "../SceneManager.h"
#include "../EditorCamera.h"
#include "SceneHierarchy.h"

#include <QuasarEngine/Entity/Entity.h>
#include <QuasarEngine/Renderer/Framebuffer.h>
#include <QuasarEngine/Renderer/Renderer.h>

#include "imgui/imgui.h"
#include <ImGuizmo.h>
#include <glm/glm.hpp>

namespace QuasarEngine
{
	class EditorViewport
	{
	public:
		EditorViewport();

		void Render(Scene& scene, EditorCamera& camera);

		void Update(EditorCamera& camera);

		void OnImGuiRender(EditorCamera& camera, SceneManager& sceneManager, SceneHierarchy& sceneHierarchy);

		Entity GetHoveredEntity() { return m_HoveredEntity; }

		bool const IsViewportFocused() const { return m_ViewportFocused; }
		bool const IsViewportHovered() const { return m_ViewportHovered; }

	private:
		void ResizeIfNeeded(EditorCamera& camera, const ImVec2& panelSize);

	private:
		std::shared_ptr<Framebuffer> m_EditorFrameBuffer;

		glm::vec2 m_EditorViewportSize = { 0.0f, 0.0f };
		glm::vec2 m_EditorViewportBounds[2];
		ImVec2    m_ViewportPanelSize = { 0, 0 };

		bool m_ViewportFocused = false;
		bool m_ViewportHovered = false;

		Entity m_HoveredEntity = {};

		int m_GizmoOperation = -1;
		ImGuizmo::MODE m_GizmoMode = ImGuizmo::LOCAL;
		bool m_SnapEnabled = false;
		float m_SnapT = 0.5f;
		float m_SnapR = 5.0f;
		float m_SnapS = 0.1f;

		double m_LastTime = Renderer::Instance().GetTime();
	};
}
