#pragma once

#include <memory>
#include <string>
#include <optional>

#include <Editor/EditorCamera.h>
#include <Editor/Modules/SceneHierarchy/SceneHierarchy.h>

#include <QuasarEngine/Renderer/Framebuffer.h>
#include <QuasarEngine/Renderer/Renderer.h>
#include <QuasarEngine/Scene/SceneManager.h>
#include <QuasarEngine/Entity/Entity.h>

#include <Editor/Modules/IEditorModule.h>

#include "imgui/imgui.h"
#include <ImGuizmo.h>
#include <glm/glm.hpp>

namespace QuasarEngine
{
	class EditorViewport : public IEditorModule
	{
	public:
		EditorViewport(EditorContext& context);
		~EditorViewport() override;

		void Update(double dt) override;
		void Render() override;
		void RenderUI() override;

		Entity GetHoveredEntity() { return m_HoveredEntity; }

		bool IsViewportFocused() const { return m_ViewportFocused; }
		bool IsViewportHovered() const { return m_ViewportHovered; }

	private:
		void ResizeIfNeeded(const ImVec2& panelSize);

		void DrawTopBar(SceneManager& sceneManager, const ImVec2& vpMin, const ImVec2& vpSize);
		void DrawStatusBar(const ImVec2& vpMin, const ImVec2& vpSize);
		void DrawAxisWidget(const ImVec2& vpMin, const ImVec2& vpSize);

		bool ToggleButton(const char* label, bool active, ImVec2 size = ImVec2(0, 0));
		void OverlayBackground(const ImVec2& min, const ImVec2& max, float rounding = 8.0f, float alpha = 0.35f);
		void Tooltip(const char* text);

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
		bool  m_SnapEnabled = false;
		float m_SnapT = 0.5f;
		float m_SnapR = 5.0f;
		float m_SnapS = 0.1f;
		float m_GizmoSizeClip = 0.16f;

		bool m_IsPlaying = false;
		bool m_IsPaused = false;

		bool m_ShowGrid = true;

		float m_CameraSpeed = 3.0f;
		float m_CameraFov = 60.0f;

		float m_LeftShelfW = 64.0f;
		float m_StatusBarH = 22.0f;

		double m_LastTime = Renderer::Instance().GetTime();
		double m_FrameTimeMs = 0.0;

		ImU32 m_ColX = IM_COL32(220, 90, 90, 255);
		ImU32 m_ColY = IM_COL32(120, 200, 120, 255);
		ImU32 m_ColZ = IM_COL32(100, 150, 240, 255);
	};
}
