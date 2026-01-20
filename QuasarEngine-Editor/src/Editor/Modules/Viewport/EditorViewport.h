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
#include <type_traits>

namespace QuasarEngine
{
	enum class ModelImportPreset
	{
		Custom = 0,
		Full,
		Static,
		QuickPreview 
	};

	struct ModelImportDialogState
	{
		bool open = false;
		bool requestOpen = false;

		std::string absolutePath;
		std::string assetId;
		std::string extension;

		ModelImportOptions options;

		ModelImportPreset preset = ModelImportPreset::Full;
		bool instantiateInScene = true;
	};

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

		void OpenModelImportDialog(const std::string& filePath);
		void DrawModelImportPopup();
		void EnsureModelReady(const ModelImportDialogState& dialog);

		ModelImportDialogState m_ModelImportDialog;

		template<typename ComponentT>
		void DrawComponentIcons(entt::registry& reg, const glm::mat4& viewProj, ImVec2 vpMin, ImVec2 vpSize, ImDrawList* dl, ImTextureID iconTex)
		{
			auto view = reg.view<TransformComponent, TagComponent, ComponentT>();
			for (auto ent : view)
			{
				auto& tr = view.get<TransformComponent>(ent);
				auto& tag = view.get<TagComponent>(ent);

				auto e = (entt::entity)ent;

				ImVec2 screen;
				if (!WorldToScreen(tr.Position, viewProj, vpMin, vpSize, screen))
					continue;

				float iconSize = 40.0f;

				DrawIconBillboard(dl, screen, iconSize, iconTex);

				ImVec2 half(iconSize * 0.5f, iconSize * 0.5f);
				ImVec2 p0(screen.x - half.x, screen.y - half.y);
				ImVec2 p1(screen.x + half.x, screen.y + half.y);

				Entity entity{ e, Renderer::Instance().m_SceneData.m_Scene->GetRegistry() };

				bool hovered = ImGui::IsMouseHoveringRect(p0, p1);
				if (hovered)
				{
					dl->AddRect(p0, p1, IM_COL32(255, 255, 255, 180), 3.0f);
					if (ImGui::IsMouseClicked(ImGuiMouseButton_Left))
					{
						m_Context.selectedEntity = entity;
					}
				}

				if (m_Context.selectedEntity == entity || hovered)
				{
					if constexpr (std::is_same_v<ComponentT, CameraComponent>)
					{
						float aspect = vpSize.x > 1.0f ? (vpSize.x / vpSize.y) : 1.0f;

						auto& cam = entity.GetComponent<CameraComponent>();

						glm::mat4 camWorld = tr.GetGlobalTransform();

						float fov = cam.FovDeg;
						float nearZ = cam.NearZ;
						float farZ = cam.FarZ;

						ImU32 colLine = IM_COL32(120, 200, 255, 180);
						ImU32 colFill = IM_COL32(120, 200, 255, 35);

						DrawFrustum(dl, viewProj, vpMin, vpSize, camWorld, fov, nearZ, farZ, aspect, colLine, colFill);
					}
				}
			}
		}

	private:
		std::shared_ptr<Framebuffer> m_EditorFrameBuffer;
		glm::vec2 m_EditorViewportSize = { 0.0f, 0.0f };
		glm::vec2 m_EditorViewportBounds[2];
		ImVec2 m_ViewportPanelSize = { 0, 0 };

		std::shared_ptr<Texture2D> m_CameraIconTex;
		std::shared_ptr<Texture2D> m_LightIconTex;

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
