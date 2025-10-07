#include "EditorViewport.h"

#include <ImGuizmo.h>
#include <imgui/imgui.h>
#include <imgui/imgui_internal.h>

#include "../Editor.h"
#include "../SceneManager.h"

#include <QuasarEngine/Core/Application.h>
#include <QuasarEngine/Core/Input.h>
#include <QuasarEngine/Renderer/RenderCommand.h>
#include <QuasarEngine/Entity/Components/TransformComponent.h>
#include <QuasarEngine/Entity/Components/HierarchyComponent.h>
#include <QuasarEngine/Tools/Math.h>

#include <glm/gtc/type_ptr.hpp>
#include <glm/gtx/matrix_decompose.hpp>
#include <glm/gtx/euler_angles.hpp>

namespace QuasarEngine
{
	EditorViewport::EditorViewport() : m_EditorViewportSize({ 0.0f, 0.0f })
	{
		FramebufferSpecification spec;
		spec.Width = Application::Get().GetWindow().GetWidth();
		spec.Height = Application::Get().GetWindow().GetHeight();

		spec.Attachments = {
			FramebufferTextureFormat::RGBA8,
			//FramebufferTextureFormat::RED_INTEGER,
			FramebufferTextureFormat::Depth
		};

		m_EditorFrameBuffer = Framebuffer::Create(spec);
		m_EditorFrameBuffer->Invalidate();
	}

	void EditorViewport::Render(Scene& scene, EditorCamera& camera)
	{
		if (!m_EditorFrameBuffer) return;

		m_EditorFrameBuffer->Bind();

		RenderCommand::Instance().ClearColor(glm::vec4(0.2f, 0.2f, 0.2f, 1.0f));
		RenderCommand::Instance().Clear();

		Renderer::Instance().BeginScene(scene);

		Renderer::Instance().RenderSkybox(camera);
		Renderer::Instance().Render(camera);
		Renderer::Instance().RenderDebug(camera);

		Renderer::Instance().EndScene();

		m_EditorFrameBuffer->Unbind();
	}

	void EditorViewport::Update(EditorCamera& camera)
	{
		ResizeIfNeeded(camera, m_ViewportPanelSize);

		double now = Renderer::Instance().GetTime();
		m_LastTime = now;

		auto mouse = ImGui::GetMousePos();
		bool mouseInViewportRect =
			mouse.x >= m_EditorViewportBounds[0].x && mouse.x < m_EditorViewportBounds[1].x &&
			mouse.y >= m_EditorViewportBounds[0].y && mouse.y < m_EditorViewportBounds[1].y;

		bool overGizmo = ImGuizmo::IsOver() || ImGuizmo::IsUsing();
		bool overItem = ImGui::IsAnyItemHovered() || ImGui::IsAnyItemActive();

		camera.m_CameraFocus = (m_ViewportHovered && mouseInViewportRect && !overGizmo && !overItem);

		if (m_ViewportFocused)
		{
			if (Input::IsKeyPressed(Key::Escape) || Input::IsKeyPressed(Key::E))
				m_GizmoOperation = -1;

			if (Input::IsKeyPressed(Key::R) || Input::IsKeyPressed(Key::W))
				m_GizmoOperation = ImGuizmo::OPERATION::TRANSLATE;

			if (Input::IsKeyPressed(Key::T) || Input::IsKeyPressed(Key::E))
				m_GizmoOperation = ImGuizmo::OPERATION::ROTATE;

			if (Input::IsKeyPressed(Key::Y) || Input::IsKeyPressed(Key::S))
				m_GizmoOperation = ImGuizmo::OPERATION::SCALE;

			if (Input::IsKeyPressed(Key::L))
				m_GizmoMode = (m_GizmoMode == ImGuizmo::LOCAL) ? ImGuizmo::WORLD : ImGuizmo::LOCAL;

			//m_SnapEnabled = Input::IsKeyPressed(Key::LeftControl) || Input::IsKeyPressed(Key::RightControl);
		}
	}

	void EditorViewport::ResizeIfNeeded(EditorCamera& camera, const ImVec2& panelSize)
	{
		uint32_t w = (uint32_t)std::max(1.0f, panelSize.x);
		uint32_t h = (uint32_t)std::max(1.0f, panelSize.y);

		if ((uint32_t)m_EditorViewportSize.x != w || (uint32_t)m_EditorViewportSize.y != h)
		{
			if (m_EditorFrameBuffer)
				m_EditorFrameBuffer->Resize(w, h);

			camera.OnResize(w, h);
			m_EditorViewportSize = { (float)w, (float)h };
		}
	}

	void EditorViewport::OnImGuiRender(EditorCamera& camera, SceneManager& sceneManager, SceneHierarchy& sceneHierarchy)
	{
		ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2{ 0, 0 });

		ImGuiWindowFlags vpFlags = ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse;
		ImGui::Begin("Editor", nullptr, vpFlags);

		m_ViewportFocused = ImGui::IsWindowFocused(ImGuiFocusedFlags_RootAndChildWindows);
		m_ViewportHovered = ImGui::IsWindowHovered(ImGuiHoveredFlags_RootAndChildWindows);

		ImVec2 viewportMin = ImGui::GetCursorScreenPos();
		ImVec2 viewportSize = ImGui::GetContentRegionAvail();
		m_ViewportPanelSize = viewportSize;

		//ResizeIfNeeded(camera, viewportSize);

		m_EditorViewportBounds[0] = { viewportMin.x, viewportMin.y };
		m_EditorViewportBounds[1] = { viewportMin.x + viewportSize.x, viewportMin.y + viewportSize.y };

		if (m_EditorFrameBuffer)
		{
			if (void* handle = m_EditorFrameBuffer->GetColorAttachment(0))
			{
				ImVec2 uv0 = (RendererAPI::GetAPI() == RendererAPI::API::OpenGL) ? ImVec2{ 0, 1 } : ImVec2{ 0, 0 };
				ImVec2 uv1 = (RendererAPI::GetAPI() == RendererAPI::API::OpenGL) ? ImVec2{ 1, 0 } : ImVec2{ 1, 1 };
				ImGui::Image((ImTextureID)handle, viewportSize, uv0, uv1);
			}
		}

		ImGuizmo::SetOrthographic(false);
		ImGuizmo::SetDrawlist();
		ImGuizmo::SetRect(viewportMin.x, viewportMin.y, viewportSize.x, viewportSize.y);

		{
			ImVec2 overlayPos = { viewportMin.x + 10.0f, viewportMin.y + 10.0f };
			ImVec2 oldCursor = ImGui::GetCursorScreenPos();
			ImGui::SetCursorScreenPos(overlayPos);

			ImGui::PushStyleColor(ImGuiCol_ChildBg, ImVec4(0, 0, 0, 0.35f));
			ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding, 8.0f);
			ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(10, 8));
			ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(10, 6));

#if IMGUI_VERSION_NUM >= 19000
			ImGuiChildFlags child_flags = ImGuiChildFlags_AutoResizeX | ImGuiChildFlags_AutoResizeY;
			ImGuiWindowFlags window_flags = ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse |
				ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_NoNavFocus |
				ImGuiWindowFlags_NoMove;
			ImGui::BeginChild("ViewportToolbar", ImVec2(0, 0), child_flags, window_flags);
#else
			ImGuiWindowFlags window_flags = ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse |
				ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_NoNavFocus |
				ImGuiWindowFlags_NoMove | ImGuiWindowFlags_AlwaysAutoResize;
			ImGui::BeginChild("ViewportToolbar", ImVec2(0, 0), false, window_flags);
#endif

			{
				auto ToggleButton = [](const char* label, bool active) -> bool {
					if (active) {
						ImGui::PushStyleColor(ImGuiCol_Button, ImGui::GetStyleColorVec4(ImGuiCol_ButtonActive));
						ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImGui::GetStyleColorVec4(ImGuiCol_ButtonHovered));
					}
					bool pressed = ImGui::Button(label);
					if (active) { ImGui::PopStyleColor(2); }
					return pressed;
					};

				bool isMove = (m_GizmoOperation == ImGuizmo::TRANSLATE);
				bool isRotate = (m_GizmoOperation == ImGuizmo::ROTATE);
				bool isScale = (m_GizmoOperation == ImGuizmo::SCALE);
				if (ToggleButton("Move", isMove))     m_GizmoOperation = ImGuizmo::TRANSLATE; ImGui::SameLine();
				if (ToggleButton("Rotate", isRotate)) m_GizmoOperation = ImGuizmo::ROTATE;    ImGui::SameLine();
				if (ToggleButton("Scale", isScale))   m_GizmoOperation = ImGuizmo::SCALE;     ImGui::SameLine();
				if (ImGui::Button("Off"))             m_GizmoOperation = -1;

#if IMGUI_VERSION_NUM >= 18900
				ImGui::SeparatorText("Gizmo");
#else
				ImGui::Separator(); ImGui::TextUnformatted("Gizmo");
#endif
				ImGui::TextUnformatted("Space:");
				ImGui::SameLine();
				bool local = (m_GizmoMode == ImGuizmo::LOCAL);
				if (ImGui::RadioButton("Local", local))  m_GizmoMode = ImGuizmo::LOCAL;
				ImGui::SameLine();
				if (ImGui::RadioButton("World", !local))  m_GizmoMode = ImGuizmo::WORLD;

				static float gizmoSize = 1.0f;
				ImGui::SetNextItemWidth(160.0f);
				if (ImGui::SliderFloat("Size", &gizmoSize, 0.1f, 0.2f, "%.2f"))
					ImGuizmo::SetGizmoSizeClipSpace(gizmoSize);

#if IMGUI_VERSION_NUM >= 18900
				ImGui::SeparatorText("Snap");
#else
				ImGui::Separator(); ImGui::TextUnformatted("Snap");
#endif

				ImGui::Checkbox("Enable", &m_SnapEnabled);

				ImGui::SameLine();
				ImGui::TextDisabled("(hold Ctrl)");

				ImGui::SetNextItemWidth(120.0f);
				ImGui::DragFloat("Snap Move", &m_SnapT, 0.05f, 0.001f, 100.0f, "%.3f");
				ImGui::SetNextItemWidth(120.0f);
				ImGui::DragFloat("Snap Rotate", &m_SnapR, 0.5f, 0.1f, 360.0f, "%.1f");
				ImGui::SetNextItemWidth(120.0f);
				ImGui::DragFloat("Snap Scale", &m_SnapS, 0.01f, 0.001f, 10.0f, "%.3f");

#if IMGUI_VERSION_NUM >= 18900
				ImGui::SeparatorText("Transform");
#else
				ImGui::Separator(); ImGui::TextUnformatted("Transform");
#endif
				if (sceneHierarchy.m_SelectedEntity.IsValid()) {
					auto& tc = sceneHierarchy.m_SelectedEntity.GetComponent<TransformComponent>();
					ImGui::SetNextItemWidth(220.0f);
					if (ImGui::DragFloat3("Position", &tc.Position.x, 0.01f)) { }
					
					glm::vec3 rotDeg = glm::degrees(tc.Rotation);
					ImGui::SetNextItemWidth(220.0f);
					if (ImGui::DragFloat3("Rotation", &rotDeg.x, 0.5f, -360.0f, 360.0f, "%.1f")) {
						tc.Rotation = glm::radians(rotDeg);
					}
					ImGui::SetNextItemWidth(220.0f);
					if (ImGui::DragFloat3("Scale", &tc.Scale.x, 0.01f, 0.0001f, 1000.0f)) { }
				}

				struct SnapState { bool active; };
				static thread_local SnapState s;
				s.active = m_SnapEnabled;
			}
			ImGui::EndChild();

			ImGui::PopStyleVar(3);
			ImGui::PopStyleColor();

			ImGui::SetCursorScreenPos(oldCursor);
		}

		if (sceneHierarchy.m_SelectedEntity.IsValid() && m_GizmoOperation != -1)
		{
			auto& tc = sceneHierarchy.m_SelectedEntity.GetComponent<TransformComponent>();
			glm::mat4 transform = tc.GetGlobalTransform();

			float snap[3] = { m_SnapT, m_SnapT, m_SnapT };
			if (m_GizmoOperation == ImGuizmo::ROTATE) snap[0] = snap[1] = snap[2] = m_SnapR;
			if (m_GizmoOperation == ImGuizmo::SCALE)  snap[0] = snap[1] = snap[2] = m_SnapS;

			const glm::mat4& view = camera.getViewMatrix();
			const glm::mat4& proj = camera.getProjectionMatrix();

			ImGuizmo::Manipulate(glm::value_ptr(view),
				glm::value_ptr(proj),
				(ImGuizmo::OPERATION)m_GizmoOperation,
				m_GizmoMode,
				glm::value_ptr(transform),
				nullptr,
				m_SnapEnabled ? snap : nullptr);

			if (ImGuizmo::IsUsing())
			{
				if (ImGuizmo::IsOver()) camera.useGuizmo(); else camera.unuseGuizmo();

				UUID parentID = sceneHierarchy.m_SelectedEntity.HasComponent<HierarchyComponent>()
					? sceneHierarchy.m_SelectedEntity.GetComponent<HierarchyComponent>().m_Parent
					: UUID::Null();

				glm::mat4 finalTransform = transform;

				while (parentID != UUID::Null())
				{
					std::optional<Entity> parent = Renderer::Instance().m_SceneData.m_Scene->GetEntityByUUID(parentID);
					if (parent.has_value())
					{
						glm::mat4 parentLocal = parent->GetComponent<TransformComponent>().GetLocalTransform();
						finalTransform = finalTransform * glm::inverse(parentLocal);
						parentID = parent->HasComponent<HierarchyComponent>()
							? parent->GetComponent<HierarchyComponent>().m_Parent
							: UUID::Null();
					}
					else break;
				}

				glm::vec3 skew, translation, scale;
				glm::vec4 perspective;
				glm::quat rotationQuat;
				if (glm::decompose(finalTransform, scale, rotationQuat, translation, skew, perspective))
				{
					tc.Position = translation;
					tc.Scale = scale;

					glm::vec3 euler = glm::eulerAngles(rotationQuat);
					euler = glm::mod(euler + glm::pi<float>(), glm::two_pi<float>()) - glm::pi<float>();
					tc.Rotation = euler;
				}
			}
			else
			{
				camera.unuseGuizmo();
			}
		}

		{
			auto [mx, my] = ImGui::GetMousePos();

			glm::vec2 vpMin = { m_EditorViewportBounds[0].x, m_EditorViewportBounds[0].y };
			glm::vec2 vpMax = { m_EditorViewportBounds[1].x, m_EditorViewportBounds[1].y };
			glm::vec2 vpSize = vpMax - vpMin;

			float localX = (float)mx - vpMin.x;
			float localY = (float)my - vpMin.y;
			
			localY = vpSize.y - localY;

			bool inside =
				localX >= 0 && localY >= 0 &&
				localX < vpSize.x && localY < vpSize.y;

			if (inside && m_ViewportHovered && m_EditorFrameBuffer)
			{
				UUID pixelEntity = UUID(m_EditorFrameBuffer->ReadPixel(1, (int)localX, (int)localY));
				if (pixelEntity != UUID::Null())
				{
					std::optional<Entity> hovered = Renderer::Instance().m_SceneData.m_Scene->GetEntityByUUID(pixelEntity);
					m_HoveredEntity = hovered.value_or(Entity{});
				}
				else
				{
					m_HoveredEntity = {};
				}
			}
			else
			{
				m_HoveredEntity = {};
			}
		}

		ImRect dropRect(
			ImVec2(m_EditorViewportBounds[0].x, m_EditorViewportBounds[0].y),
			ImVec2(m_EditorViewportBounds[1].x, m_EditorViewportBounds[1].y)
		);
		if (ImGui::BeginDragDropTargetCustom(dropRect, ImGui::GetID("EditorViewportDrop")))
		{
			if (const ImGuiPayload* payload = ImGui::AcceptDragDropPayload("CONTENT_BROWSER_ITEM"))
			{
				const wchar_t* path = (const wchar_t*)payload->Data;
				std::wstring ws(path);
				std::string filePath(ws.begin(), ws.end());
				const size_t slash = filePath.find_last_of("/\\");
				std::string selectedFile = filePath.substr(slash + 1);
				std::string fileExtension = selectedFile.substr(selectedFile.find_last_of(".") + 1);

				if (fileExtension == "obj" || fileExtension == "dae" || fileExtension == "fbx"
					|| fileExtension == "glb" || fileExtension == "gltf" || fileExtension == "bin")
					sceneManager.AddGameObject(filePath);
				else if (fileExtension == "scene")
					sceneManager.LoadScene(filePath);
			}
			ImGui::EndDragDropTarget();
		}

		ImGui::End();
		ImGui::PopStyleVar();

		// ---- (optionnel) petit debug overlay ----
		/*
		ImGui::Begin("Editor infos");
		ImGui::Text("Focused: %s", m_ViewportFocused ? "yes" : "no");
		ImGui::Text("Hovered: %s", m_ViewportHovered ? "yes" : "no");
		ImGui::Text("Viewport: %.0fx%.0f", m_EditorViewportSize.x, m_EditorViewportSize.y);
		ImGui::End();
		*/
	}
}
