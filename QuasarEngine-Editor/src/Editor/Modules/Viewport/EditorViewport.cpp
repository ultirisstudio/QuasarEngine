#include "EditorViewport.h"

#include <ImGuizmo.h>
#include <imgui/imgui.h>
#include <imgui/imgui_internal.h>

#include <Editor/Editor.h>
#include <Editor/SceneManager.h>

#include <QuasarEngine/Core/Application.h>
#include <QuasarEngine/Core/Input.h>
#include <QuasarEngine/Renderer/RenderCommand.h>
#include <QuasarEngine/Entity/Components/TransformComponent.h>
#include <QuasarEngine/Entity/Components/HierarchyComponent.h>

#include <glm/gtc/type_ptr.hpp>
#include <glm/gtx/matrix_decompose.hpp>
#include <glm/gtx/euler_angles.hpp>

namespace QuasarEngine
{
	bool EditorViewport::ToggleButton(const char* label, bool active, ImVec2 size)
	{
		if (active) {
			ImGui::PushStyleColor(ImGuiCol_Button, ImGui::GetStyleColorVec4(ImGuiCol_ButtonActive));
			ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImGui::GetStyleColorVec4(ImGuiCol_ButtonHovered));
		}
		bool pressed = ImGui::Button(label, size);
		if (active) ImGui::PopStyleColor(2);
		return pressed;
	}

	void EditorViewport::OverlayBackground(const ImVec2& min, const ImVec2& max, float rounding, float alpha)
	{
		ImDrawList* dl = ImGui::GetWindowDrawList();
		ImU32 bg = ImGui::ColorConvertFloat4ToU32(ImVec4(0, 0, 0, alpha));
		dl->AddRectFilled(min, max, bg, rounding);
		dl->AddRect(min, max, ImGui::GetColorU32(ImVec4(1, 1, 1, 0.08f)), rounding);
	}

	void EditorViewport::Tooltip(const char* text)
	{
		if (ImGui::IsItemHovered(ImGuiHoveredFlags_DelayNormal)) {
			ImGui::BeginTooltip();
			ImGui::TextUnformatted(text);
			ImGui::EndTooltip();
		}
	}

	EditorViewport::EditorViewport()
	{
		FramebufferSpecification spec;
		spec.Width = Application::Get().GetWindow().GetWidth();
		spec.Height = Application::Get().GetWindow().GetHeight();
		spec.Attachments = {
			FramebufferTextureFormat::RGBA8,
			// FramebufferTextureFormat::RED_INTEGER,
			FramebufferTextureFormat::Depth
		};

		m_EditorFrameBuffer = Framebuffer::Create(spec);
		m_EditorFrameBuffer->Invalidate();
	}

	void EditorViewport::Render(Scene& scene, EditorCamera& camera)
	{
		if (!m_EditorFrameBuffer) return;

		m_EditorFrameBuffer->Bind();
		const auto& spec = m_EditorFrameBuffer->GetSpecification();
		RenderCommand::Instance().SetViewport(0, 0, spec.Width, spec.Height);

		RenderCommand::Instance().ClearColor(glm::vec4(0.2f, 0.2f, 0.2f, 1.0f));
		RenderCommand::Instance().Clear();

		Renderer::Instance().BeginScene(scene);
		Renderer::Instance().RenderSkybox(camera);
		Renderer::Instance().Render(camera);
		if (m_ShowGrid) Renderer::Instance().RenderDebug(camera);
		Renderer::Instance().EndScene();

		m_EditorFrameBuffer->Unbind();
	}

	void EditorViewport::Update(EditorCamera& camera)
	{
		ResizeIfNeeded(camera, m_ViewportPanelSize);

		double now = Renderer::Instance().GetTime();
		m_FrameTimeMs = (now - m_LastTime) * 1000.0;
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
			if (Input::IsKeyPressed(Key::Q)) m_GizmoOperation = -1;
			if (Input::IsKeyPressed(Key::W)) m_GizmoOperation = ImGuizmo::TRANSLATE;
			if (Input::IsKeyPressed(Key::E)) m_GizmoOperation = ImGuizmo::ROTATE;
			if (Input::IsKeyPressed(Key::R)) m_GizmoOperation = ImGuizmo::SCALE;

			if (Input::IsKeyPressed(Key::L))
				m_GizmoMode = (m_GizmoMode == ImGuizmo::LOCAL) ? ImGuizmo::WORLD : ImGuizmo::LOCAL;
		}
	}

	void EditorViewport::ResizeIfNeeded(EditorCamera& camera, const ImVec2& panelSize)
	{
		uint32_t w = (uint32_t)std::max(1.0f, panelSize.x);
		uint32_t h = (uint32_t)std::max(1.0f, panelSize.y);

		if ((uint32_t)m_EditorViewportSize.x != w || (uint32_t)m_EditorViewportSize.y != h)
		{
			if (m_EditorFrameBuffer) m_EditorFrameBuffer->Resize(w, h);
			camera.OnResize(w, h);
			m_EditorViewportSize = { (float)w, (float)h };
		}
	}

	void EditorViewport::DrawTopBar(EditorCamera& camera, SceneManager& sceneManager, const ImVec2& vpMin, const ImVec2& vpSize)
	{
		const float kPad = 8.0f;
		const float kBtnH = 30.0f;
		const float kBtnWBig = 84.0f;
		const float kSpacing = ImGui::GetStyle().ItemSpacing.x;

		ImVec2 leftStart = ImVec2(vpMin.x + kPad, vpMin.y + kPad);
		ImGui::SetCursorScreenPos(leftStart);
		ImGui::BeginGroup();
		{
			if (ToggleButton("Select", m_GizmoOperation == -1, ImVec2(50, kBtnH))) m_GizmoOperation = -1; ImGui::SameLine(0, kSpacing);
			if (ToggleButton("Move", m_GizmoOperation == ImGuizmo::TRANSLATE, ImVec2(48, kBtnH))) m_GizmoOperation = ImGuizmo::TRANSLATE; ImGui::SameLine(0, kSpacing);
			if (ToggleButton("Rotate", m_GizmoOperation == ImGuizmo::ROTATE, ImVec2(50, kBtnH))) m_GizmoOperation = ImGuizmo::ROTATE;    ImGui::SameLine(0, kSpacing);
			if (ToggleButton("Scale", m_GizmoOperation == ImGuizmo::SCALE, ImVec2(54, kBtnH))) m_GizmoOperation = ImGuizmo::SCALE;

			ImGui::SameLine(0, 18.0f);
			bool isLocal = (m_GizmoMode == ImGuizmo::LOCAL);
			if (ImGui::RadioButton("Local", isLocal))  m_GizmoMode = ImGuizmo::LOCAL;
			ImGui::SameLine();
			if (ImGui::RadioButton("World", !isLocal)) m_GizmoMode = ImGuizmo::WORLD;

			ImGui::SameLine(0, 18.0f);
			if (ToggleButton("Magnet", m_SnapEnabled, ImVec2(50, kBtnH))) m_SnapEnabled = !m_SnapEnabled;

			ImGui::SameLine(0, 18.0f);
			if (ToggleButton("Grid", m_ShowGrid, ImVec2(40, kBtnH))) m_ShowGrid = !m_ShowGrid;
		}
		ImGui::EndGroup();
		ImVec2 leftEnd = ImGui::GetItemRectMax();

		float runW = 4 * kBtnWBig + 3 * kSpacing;
		ImVec2 centerStart = ImVec2(vpMin.x + (vpSize.x - runW) * 0.5f, vpMin.y + kPad);
		ImGui::SetCursorScreenPos(centerStart);
		ImGui::BeginGroup();
		{
			if (ToggleButton("Play", m_IsPlaying && !m_IsPaused, ImVec2(kBtnWBig, kBtnH))) {
				m_IsPlaying = true; 
				m_IsPaused = false;

				sceneManager.SaveScene();
				sceneManager.GetActiveScene().OnRuntimeStart();
			}

			ImGui::SameLine(0, kSpacing);

			if (ToggleButton("Pause", m_IsPlaying && m_IsPaused, ImVec2(kBtnWBig, kBtnH))) {
				if (m_IsPlaying) {
					m_IsPaused = !m_IsPaused;
				}

				//TODO : Pause scene
			}

			ImGui::SameLine(0, kSpacing);

			if (ImGui::Button("Stop", ImVec2(kBtnWBig, kBtnH))) {
				m_IsPlaying = false;
				m_IsPaused = false;

				sceneManager.GetActiveScene().OnRuntimeStop();
				sceneManager.ReloadScene(sceneManager.GetSceneObject().GetPath());
			}

			ImGui::SameLine(0, kSpacing);

			ImGui::BeginDisabled(!m_IsPlaying);
			if (ImGui::Button("Step", ImVec2(kBtnWBig, kBtnH))) {
				// TODO : Step scene
				m_IsPaused = true;
			}
			ImGui::EndDisabled();
		}
		ImGui::EndGroup();
		ImVec2 centerEnd = ImGui::GetItemRectMax();

		ImVec2 rightSize = ImVec2(260, kBtnH);
		ImVec2 rightStart = ImVec2(vpMin.x + vpSize.x - rightSize.x - kPad, vpMin.y + kPad);
		ImGui::SetCursorScreenPos(rightStart);
		ImGui::BeginGroup();
		{
			ImGui::SetNextItemWidth(vpMin.x + vpSize.x - 60);
			if (ImGui::Button("Settings", ImVec2(60, kBtnH))) ImGui::OpenPopup("QuickSettings##Viewport");
			if (ImGui::BeginPopup("QuickSettings##Viewport")) {
				ImGui::TextDisabled("Snap");
				ImGui::Checkbox("Enable Snap", &m_SnapEnabled);
				ImGui::SetNextItemWidth(140.0f); ImGui::DragFloat("Move", &m_SnapT, 0.05f, 0.001f, 100.0f, "%.3f");
				ImGui::SetNextItemWidth(140.0f); ImGui::DragFloat("Rotate", &m_SnapR, 0.5f, 0.1f, 360.0f, "%.1f");
				ImGui::SetNextItemWidth(140.0f); ImGui::DragFloat("Scale", &m_SnapS, 0.01f, 0.001f, 10.0f, "%.3f");
				ImGui::SetNextItemWidth(160.0f); ImGui::SliderFloat("Gizmo Size", &m_GizmoSizeClip, 0.08f, 0.25f, "%.2f");

				ImGui::Separator();
				ImGui::TextDisabled("Camera");
				ImGui::SetNextItemWidth(140.0f); ImGui::DragFloat("Speed", &m_CameraSpeed, 0.1f, 0.1f, 100.0f);
				ImGui::SetNextItemWidth(140.0f); ImGui::DragFloat("FOV", &m_CameraFov, 0.1f, 1.0f, 120.0f);
				ImGui::EndPopup();
			}
		}
		ImGui::EndGroup();
		ImVec2 rightEnd = ImGui::GetItemRectMax();

		float yMax = std::max(std::max(leftEnd.y, centerEnd.y), rightEnd.y) + kPad;
		//OverlayBackground(ImVec2(vpMin.x + 4.0f, vpMin.y + 4.0f), ImVec2(vpMin.x + vpSize.x - 4.0f, yMax), 8.0f, 0.33f);
	}

	void EditorViewport::DrawStatusBar(const ImVec2& vpMin, const ImVec2& vpSize)
	{
		ImVec2 min = ImVec2(vpMin.x + 4.0f, vpMin.y + vpSize.y - m_StatusBarH - 4.0f);
		ImVec2 max = ImVec2(vpMin.x + vpSize.x - 4.0f, vpMin.y + vpSize.y - 4.0f);
		//OverlayBackground(min, max, 6.0f, 0.30f);

		ImGui::SetCursorScreenPos(ImVec2(min.x + 8.0f, min.y + 2.0f));
		if (m_HoveredEntity.IsValid()) ImGui::Text("Hover: %s", m_HoveredEntity.GetName().c_str());
		else                            ImGui::Text("Hover: <none>");

		ImGui::SameLine();
		ImGui::SetCursorPosX(max.x - 220.0f);
		ImGui::Text("Frame: %.2f ms (%.0f FPS)", m_FrameTimeMs, (m_FrameTimeMs > 0.0 ? 1000.0f / (float)m_FrameTimeMs : 0.0f));
	}

	void EditorViewport::DrawAxisWidget(const ImVec2& vpMin, const ImVec2& vpSize)
	{
		const float w = 72.0f, h = 72.0f;
		ImVec2 pad = ImVec2(0.0f, 0.0f);
		ImVec2 pos = ImVec2(vpMin.x + vpSize.x + pad.x - w - 8.0f, vpMin.y + pad.y + 8.0f);

		ImDrawList* dl = ImGui::GetWindowDrawList();
		ImVec2 c = ImVec2(pos.x + w * 0.5f, pos.y + h * 0.5f);

		//OverlayBackground(pos, ImVec2(pos.x + w, pos.y + h), 6.0f, 0.25f);

		const float len = 24.0f;
		dl->AddLine(c, ImVec2(c.x + len, c.y), m_ColX, 2.0f);
		dl->AddLine(c, ImVec2(c.x, c.y - len), m_ColY, 2.0f);
		dl->AddLine(c, ImVec2(c.x - len * 0.7f, c.y + len * 0.7f), m_ColZ, 2.0f);

		dl->AddText(ImVec2(c.x + len + 4, c.y - 6), m_ColX, "X");
		dl->AddText(ImVec2(c.x - 6, c.y - len - 12), m_ColY, "Y");
		dl->AddText(ImVec2(c.x - len * 0.7f - 12, c.y + len * 0.7f - 6), m_ColZ, "Z");
	}

	void EditorViewport::OnImGuiRender(EditorCamera& camera, SceneManager& sceneManager, SceneHierarchy& sceneHierarchy)
	{
		ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2{ 0, 0 });
		ImGui::Begin("Editor", nullptr, ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse);

		m_ViewportFocused = ImGui::IsWindowFocused(ImGuiFocusedFlags_RootAndChildWindows);
		m_ViewportHovered = ImGui::IsWindowHovered(ImGuiHoveredFlags_RootAndChildWindows);

		ImVec2 vpMin = ImGui::GetCursorScreenPos();
		ImVec2 vpSize = ImGui::GetContentRegionAvail();
		m_ViewportPanelSize = vpSize;

		m_EditorViewportBounds[0] = { vpMin.x, vpMin.y };
		m_EditorViewportBounds[1] = { vpMin.x + vpSize.x, vpMin.y + vpSize.y };

		if (m_EditorFrameBuffer) {
			if (void* handle = m_EditorFrameBuffer->GetColorAttachment(0)) {
				ImVec2 uv0 = (RendererAPI::GetAPI() == RendererAPI::API::OpenGL) ? ImVec2{ 0, 1 } : ImVec2{ 0, 0 };
				ImVec2 uv1 = (RendererAPI::GetAPI() == RendererAPI::API::OpenGL) ? ImVec2{ 1, 0 } : ImVec2{ 1, 1 };
				ImGui::Image((ImTextureID)handle, vpSize, uv0, uv1);
			}
		}

		ImGuizmo::SetOrthographic(false);
		ImGuizmo::SetDrawlist();
		ImGuizmo::SetRect(vpMin.x, vpMin.y, vpSize.x, vpSize.y);

		DrawTopBar(camera, sceneManager, vpMin, vpSize);
		DrawAxisWidget(vpMin, vpSize);
		DrawStatusBar(vpMin, vpSize);

		if (sceneHierarchy.m_SelectedEntity.IsValid() && m_GizmoOperation != -1)
		{
			auto& tc = sceneHierarchy.m_SelectedEntity.GetComponent<TransformComponent>();
			glm::mat4 transform = tc.GetGlobalTransform();

			float snap[3] = { m_SnapT, m_SnapT, m_SnapT };
			if (m_GizmoOperation == ImGuizmo::ROTATE) snap[0] = snap[1] = snap[2] = m_SnapR;
			if (m_GizmoOperation == ImGuizmo::SCALE)  snap[0] = snap[1] = snap[2] = m_SnapS;

			ImGuizmo::SetGizmoSizeClipSpace(m_GizmoSizeClip);

			const glm::mat4& view = camera.getViewMatrix();
			const glm::mat4& proj = camera.getProjectionMatrix();

			bool useSnap = m_SnapEnabled || ImGui::GetIO().KeyCtrl;

			ImGuizmo::Manipulate(glm::value_ptr(view),
				glm::value_ptr(proj),
				(ImGuizmo::OPERATION)m_GizmoOperation,
				m_GizmoMode,
				glm::value_ptr(transform),
				nullptr,
				useSnap ? snap : nullptr);

			if (ImGuizmo::IsUsing())
			{
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
		}

		{
			auto [mx, my] = ImGui::GetMousePos();
			glm::vec2 min = { m_EditorViewportBounds[0].x, m_EditorViewportBounds[0].y };
			glm::vec2 max = { m_EditorViewportBounds[1].x, m_EditorViewportBounds[1].y };
			glm::vec2 sz = max - min;

			float lx = (float)mx - min.x;
			float ly = (float)my - min.y;
			ly = sz.y - ly;

			bool inside = lx >= 0 && ly >= 0 && lx < sz.x && ly < sz.y;
			if (inside && m_ViewportHovered && m_EditorFrameBuffer)
			{
				UUID pixelEntity = UUID(m_EditorFrameBuffer->ReadPixel(1, (int)lx, (int)ly));
				if (pixelEntity != UUID::Null()) {
					std::optional<Entity> hovered = Renderer::Instance().m_SceneData.m_Scene->GetEntityByUUID(pixelEntity);
					m_HoveredEntity = hovered.value_or(Entity{});
				}
				else {
					m_HoveredEntity = {};
				}
			}
			else m_HoveredEntity = {};
		}

		{
			ImRect dropRect(
				ImVec2(m_EditorViewportBounds[0].x, m_EditorViewportBounds[0].y),
				ImVec2(m_EditorViewportBounds[1].x, m_EditorViewportBounds[1].y)
			);

			if (ImGui::BeginDragDropTargetCustom(dropRect, ImGui::GetID("EditorViewportDrop")))
			{
				const ImGuiPayload* payload = ImGui::AcceptDragDropPayload("CONTENT_BROWSER_ITEM");
				if (!payload)
					payload = ImGui::AcceptDragDropPayload("EXTERNAL_FILE");

				if (payload && payload->Data && payload->DataSize > 0)
				{
					std::string filePath;
#if defined(_WIN32)
					const wchar_t* wpath = (const wchar_t*)payload->Data;
					std::wstring ws(wpath);
					filePath.assign(ws.begin(), ws.end());
#else
					const char* cpath = (const char*)payload->Data;
					filePath = cpath;
#endif

					std::string ext;
					if (size_t dot = filePath.find_last_of('.'); dot != std::string::npos) {
						ext = filePath.substr(dot + 1);
						for (char& ch : ext) ch = (char)std::tolower((unsigned char)ch);
					}

					if (ext == "obj" || ext == "dae" || ext == "fbx" || ext == "glb" || ext == "gltf" || ext == "bin")
					{
						sceneManager.AddGameObject(filePath);
					}
					
					else if (ext == "scene")
					{
						sceneManager.LoadScene(filePath);
					}
				}

				ImGui::EndDragDropTarget();
			}
		}

		ImGui::End();
		ImGui::PopStyleVar();
	}
}
