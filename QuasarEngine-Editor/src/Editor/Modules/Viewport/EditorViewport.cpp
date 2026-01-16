#include "EditorViewport.h"

#include <ImGuizmo.h>
#include <imgui/imgui.h>
#include <imgui/imgui_internal.h>

#include <Editor/Editor.h>

#include <QuasarEngine/Core/Application.h>
#include <QuasarEngine/Core/Input.h>
#include <QuasarEngine/Scene/SceneManager.h>
#include <QuasarEngine/Renderer/RenderCommand.h>
#include <QuasarEngine/Renderer/Renderer.h>
#include <QuasarEngine/Resources/Model.h>
#include <QuasarEngine/Entity/Components/TransformComponent.h>
#include <QuasarEngine/Entity/Components/HierarchyComponent.h>
#include <QuasarEngine/Entity/Components/CameraComponent.h>
#include <QuasarEngine/Entity/Components/LightComponent.h>

#include <glm/gtc/type_ptr.hpp>
#include <glm/gtx/matrix_decompose.hpp>
#include <glm/gtx/euler_angles.hpp>

namespace QuasarEngine
{
	namespace
	{
		const char* GetPresetLabel(ModelImportPreset preset)
		{
			switch (preset)
			{
			case ModelImportPreset::Custom:       return "Personnalise";
			case ModelImportPreset::Full:         return "Complet";
			case ModelImportPreset::Static:       return "Statique";
			case ModelImportPreset::QuickPreview: return "Preview rapide";
			}
			return "Inconnu";
		}

		void ApplyPreset(ModelImportOptions& opts, ModelImportPreset preset)
		{
			switch (preset)
			{
			case ModelImportPreset::Full:
				opts.buildMeshes = true;
				opts.loadMaterials = true;
				opts.loadSkinning = true;
				opts.loadAnimations = true;
				opts.loadTangents = true;

				opts.triangulate = true;
				opts.joinIdenticalVertices = true;
				opts.improveCacheLocality = true;
				opts.genUVIfMissing = true;
				opts.generateNormals = false;
				opts.generateTangents = true;
				break;

			case ModelImportPreset::Static:
				opts.buildMeshes = true;
				opts.loadMaterials = true;
				opts.loadSkinning = false;
				opts.loadAnimations = false;
				opts.loadTangents = true;

				opts.triangulate = true;
				opts.joinIdenticalVertices = true;
				opts.improveCacheLocality = true;
				opts.genUVIfMissing = true;
				opts.generateNormals = false;
				opts.generateTangents = true;
				break;

			case ModelImportPreset::QuickPreview:
				opts.buildMeshes = true;
				opts.loadMaterials = false;
				opts.loadSkinning = false;
				opts.loadAnimations = false;
				opts.loadTangents = false;

				opts.triangulate = false;
				opts.joinIdenticalVertices = false;
				opts.improveCacheLocality = false;
				opts.genUVIfMissing = false;
				opts.generateNormals = false;
				opts.generateTangents = false;
				break;

			case ModelImportPreset::Custom:
				break;
			}
		}

		struct DrawModeItem { const char* label; DrawMode mode; };

		static constexpr std::array<DrawModeItem, 3> kDrawModes{ {
			{ "Triangles", DrawMode::TRIANGLES },
			{ "Points",    DrawMode::POINTS    },
			{ "Lignes",    DrawMode::LINES     }
		} };

		int GetDrawModeIndex(DrawMode mode)
		{
			for (int i = 0; i < (int)kDrawModes.size(); ++i)
				if (kDrawModes[i].mode == mode)
					return i;
			return 0;
		}

		static bool WorldToScreen(const glm::vec3& world, const glm::mat4& viewProj, const ImVec2& vpPos, const ImVec2& vpSize, ImVec2& out)
		{
			glm::vec4 clip = viewProj * glm::vec4(world, 1.0f);
			if (clip.w <= 0.0001f) return false;

			glm::vec3 ndc = glm::vec3(clip) / clip.w;

			if (ndc.x < -1.2f || ndc.x > 1.2f || ndc.y < -1.2f || ndc.y > 1.2f) return false;

			out.x = vpPos.x + (ndc.x * 0.5f + 0.5f) * vpSize.x;
			out.y = vpPos.y + (1.0f - (ndc.y * 0.5f + 0.5f)) * vpSize.y;
			return true;
		}

		static void DrawIconBillboard(ImDrawList* dl, ImVec2 center, float size, ImTextureID tex, ImU32 tint = IM_COL32_WHITE)
		{
			ImVec2 half(size * 0.5f, size * 0.5f);
			ImVec2 p0(center.x - half.x, center.y - half.y);
			ImVec2 p1(center.x + half.x, center.y + half.y);

			dl->AddImage(tex, p0, p1, ImVec2(0, 0), ImVec2(1, 1), tint);

			//dl->AddRect(p0, p1, IM_COL32(0, 0, 0, 120), 3.0f);
		}

		static bool Project(ImVec2& out, const glm::vec3& wpos,
			const glm::mat4& viewProj,
			const ImVec2& vpMin, const ImVec2& vpSize)
		{
			return WorldToScreen(wpos, viewProj, vpMin, vpSize, out);
		}

		static void DrawFrustum(ImDrawList* dl, const glm::mat4& editorViewProj, const ImVec2& vpMin, const ImVec2& vpSize, const glm::mat4& camWorld, float fovYDegrees, float nearZ, float farZ, float aspect, ImU32 colLine, ImU32 colFill = 0)
		{
			glm::vec3 pos = glm::vec3(camWorld[3]);
			glm::vec3 right = glm::normalize(glm::vec3(camWorld[0]));
			glm::vec3 up = glm::normalize(glm::vec3(camWorld[1]));
			glm::vec3 fwd = -glm::normalize(glm::vec3(camWorld[2]));

			float fovY = glm::radians(fovYDegrees);
			float tanHalf = tanf(fovY * 0.5f);

			float nh = 2.0f * nearZ * tanHalf;
			float nw = nh * aspect;
			float fh = 2.0f * farZ * tanHalf;
			float fw = fh * aspect;

			glm::vec3 nc = pos + fwd * nearZ;
			glm::vec3 fc = pos + fwd * farZ;

			glm::vec3 ntl = nc + up * (nh * 0.5f) - right * (nw * 0.5f);
			glm::vec3 ntr = nc + up * (nh * 0.5f) + right * (nw * 0.5f);
			glm::vec3 nbl = nc - up * (nh * 0.5f) - right * (nw * 0.5f);
			glm::vec3 nbr = nc - up * (nh * 0.5f) + right * (nw * 0.5f);

			glm::vec3 ftl = fc + up * (fh * 0.5f) - right * (fw * 0.5f);
			glm::vec3 ftr = fc + up * (fh * 0.5f) + right * (fw * 0.5f);
			glm::vec3 fbl = fc - up * (fh * 0.5f) - right * (fw * 0.5f);
			glm::vec3 fbr = fc - up * (fh * 0.5f) + right * (fw * 0.5f);

			auto drawEdge = [&](const glm::vec3& a, const glm::vec3& b)
				{
					ImVec2 pa, pb;
					if (Project(pa, a, editorViewProj, vpMin, vpSize) &&
						Project(pb, b, editorViewProj, vpMin, vpSize))
					{
						dl->AddLine(pa, pb, colLine, 1.0f);
					}
				};

			drawEdge(ntl, ntr); drawEdge(ntr, nbr); drawEdge(nbr, nbl); drawEdge(nbl, ntl);
			drawEdge(ftl, ftr); drawEdge(ftr, fbr); drawEdge(fbr, fbl); drawEdge(fbl, ftl);
			drawEdge(ntl, ftl); drawEdge(ntr, ftr); drawEdge(nbl, fbl); drawEdge(nbr, fbr);

			if (colFill != 0)
			{
				ImVec2 p0, p1, p2, p3;
				if (Project(p0, ftl, editorViewProj, vpMin, vpSize) &&
					Project(p1, ftr, editorViewProj, vpMin, vpSize) &&
					Project(p2, fbr, editorViewProj, vpMin, vpSize) &&
					Project(p3, fbl, editorViewProj, vpMin, vpSize))
				{
					dl->AddConvexPolyFilled(&p0, 4, colFill);
				}
			}
		}
	}

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

	EditorViewport::EditorViewport(EditorContext& context) : IEditorModule(context)
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

		TextureSpecification texSpec;
		texSpec.alpha = true;

		m_CameraIconTex = Texture2D::Create(texSpec);
		m_CameraIconTex->LoadFromPath("Assets/Textures/camera.png");

		m_LightIconTex = Texture2D::Create(texSpec);
		m_LightIconTex->LoadFromPath("Assets/Textures/light.png");
	}

	EditorViewport::~EditorViewport()
	{

	}

	void EditorViewport::Render()
	{
		if (!m_EditorFrameBuffer) return;

		Scene& scene = m_Context.sceneManager->GetActiveScene();
		EditorCamera& camera = *m_Context.editorCamera;

		Renderer::Instance().BeginScene(scene);

		//Renderer::Instance().CollectLights(scene);
		Renderer::Instance().BuildLight(camera);

		m_EditorFrameBuffer->Bind();
		const auto& spec = m_EditorFrameBuffer->GetSpecification();
		RenderCommand::Instance().SetViewport(0, 0, spec.Width, spec.Height);

		//RenderCommand::Instance().ClearColor(glm::vec4(0.8f, 0.2f, 0.2f, 1.0f));
		//RenderCommand::Instance().Clear();

		m_EditorFrameBuffer->ClearColor(0.1f, 0.8f, 0.1f, 1.0f);
		m_EditorFrameBuffer->ClearDepth(1.0f);

		Renderer::Instance().RenderSkybox(camera);
		Renderer::Instance().Render(camera);
		//if (m_ShowGrid) Renderer::Instance().RenderDebug(camera);
		Renderer::Instance().EndScene();

		m_EditorFrameBuffer->Unbind();
	}

	void EditorViewport::Update(double dt)
	{
		EditorCamera& camera = *m_Context.editorCamera;

		ResizeIfNeeded(m_ViewportPanelSize);

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

	void EditorViewport::ResizeIfNeeded(const ImVec2& panelSize)
	{
		EditorCamera& camera = *m_Context.editorCamera;

		uint32_t w = (uint32_t)std::max(1.0f, panelSize.x);
		uint32_t h = (uint32_t)std::max(1.0f, panelSize.y);

		if ((uint32_t)m_EditorViewportSize.x != w || (uint32_t)m_EditorViewportSize.y != h)
		{
			if (m_EditorFrameBuffer) m_EditorFrameBuffer->Resize(w, h);
			camera.OnResize(w, h);
			m_EditorViewportSize = { (float)w, (float)h };
		}
	}

	void EditorViewport::DrawTopBar(SceneManager& sceneManager, const ImVec2& vpMin, const ImVec2& vpSize)
	{
		const float kPad = 8.0f;
		const float kBtnH = 30.0f;
		const float kBtnWBig = 84.0f;
		const float kSpacing = ImGui::GetStyle().ItemSpacing.x;

		EditorCamera& camera = *m_Context.editorCamera;

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

	void EditorViewport::RenderUI()
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
			/*m_EditorFrameBuffer->Resolve();
			auto tex = m_EditorFrameBuffer->GetColorAttachmentTexture(0);
			ImTextureID id = (ImTextureID)(intptr_t)tex->GetHandle();
			if (tex)
			{
				ImVec2 uv0 = (RendererAPI::GetAPI() == RendererAPI::API::OpenGL) ? ImVec2{ 0, 1 } : ImVec2{ 0, 0 };
				ImVec2 uv1 = (RendererAPI::GetAPI() == RendererAPI::API::OpenGL) ? ImVec2{ 1, 0 } : ImVec2{ 1, 1 };
				ImGui::Image(id, vpSize, uv0, uv1);
			}*/
			m_EditorFrameBuffer->Resolve();
			//if (void* handle = Renderer::Instance().m_SceneData.m_DirShadowFBO[0]->GetDepthAttachment()) {
			//if (auto& texture = Renderer::Instance().m_SceneData.m_DirShadowFBO[0]->GetDepthAttachmentTexture()) {
			if (void* handle = m_EditorFrameBuffer->GetColorAttachment(0)) {
				ImVec2 uv0 = (RendererAPI::GetAPI() == RendererAPI::API::OpenGL) ? ImVec2{ 0, 1 } : ImVec2{ 0, 0 };
				ImVec2 uv1 = (RendererAPI::GetAPI() == RendererAPI::API::OpenGL) ? ImVec2{ 1, 0 } : ImVec2{ 1, 1 };
				ImGui::Image((ImTextureID)handle, vpSize, uv0, uv1);
			}
		}

		ImGuizmo::SetOrthographic(false);
		ImGuizmo::SetDrawlist();
		ImGuizmo::SetRect(vpMin.x, vpMin.y, vpSize.x, vpSize.y);

		Scene& scene = m_Context.sceneManager->GetActiveScene();
		EditorCamera& camera = *m_Context.editorCamera;
		SceneManager& sceneManager = *m_Context.sceneManager;

		const glm::mat4 viewProj = camera.getProjectionMatrix() * camera.getViewMatrix();

		ImGuiViewport* vp = ImGui::GetMainViewport();

		ImDrawList* dl = ImGui::GetForegroundDrawList(vp);

		auto& registry = Renderer::Instance().m_SceneData.m_Scene->GetRegistry()->GetRegistry();
		DrawComponentIcons<CameraComponent>(registry, viewProj, vpMin, vpSize, dl, (ImTextureID)m_CameraIconTex->GetHandle());
		DrawComponentIcons<LightComponent>(registry, viewProj, vpMin, vpSize, dl, (ImTextureID)m_LightIconTex->GetHandle());

		DrawTopBar(sceneManager, vpMin, vpSize);
		DrawAxisWidget(vpMin, vpSize);
		DrawStatusBar(vpMin, vpSize);

		if (m_Context.selectedEntity.IsValid() && m_GizmoOperation != -1)
		{
			auto& tc = m_Context.selectedEntity.GetComponent<TransformComponent>();
			glm::mat4 transform = tc.GetGlobalTransform();

			float snap[3] = { m_SnapT, m_SnapT, m_SnapT };
			if (m_GizmoOperation == ImGuizmo::ROTATE) snap[0] = snap[1] = snap[2] = m_SnapR;
			if (m_GizmoOperation == ImGuizmo::SCALE)  snap[0] = snap[1] = snap[2] = m_SnapS;

			ImGuizmo::SetGizmoSizeClipSpace(m_GizmoSizeClip);

			const glm::mat4& view = camera.getViewMatrix();
			const glm::mat4& proj = camera.getProjectionMatrix();

			bool useSnap = m_SnapEnabled || ImGui::GetIO().KeyCtrl;

			ImGuizmo::Manipulate(glm::value_ptr(view), glm::value_ptr(proj), (ImGuizmo::OPERATION)m_GizmoOperation, m_GizmoMode, glm::value_ptr(transform), nullptr, useSnap ? snap : nullptr);

			if (ImGuizmo::IsUsing())
			{
				UUID parentID = m_Context.selectedEntity.HasComponent<HierarchyComponent>() ? m_Context.selectedEntity.GetComponent<HierarchyComponent>().m_Parent : UUID::Null();

				glm::mat4 finalTransform = transform;
				while (parentID != UUID::Null())
				{
					std::optional<Entity> parent = Renderer::Instance().m_SceneData.m_Scene->GetEntityByUUID(parentID);
					if (parent.has_value())
					{
						glm::mat4 parentLocal = parent->GetComponent<TransformComponent>().GetLocalTransform();
						finalTransform = finalTransform * glm::inverse(parentLocal);
						parentID = parent->HasComponent<HierarchyComponent>() ? parent->GetComponent<HierarchyComponent>().m_Parent : UUID::Null();
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

						std::cout << "Dropped file: " << filePath << " (." << ext << ")\n";

						bool isModelExt =
							(ext == "obj" || ext == "dae" || ext == "fbx" ||
								ext == "glb" || ext == "gltf" || ext == "bin" || ext == "ply");

						if (isModelExt)
						{
							OpenModelImportDialog(filePath);
						}
						else if (ext == "scene")
						{
							sceneManager.LoadScene(filePath);
						}
					}

					ImGui::EndDragDropTarget();
				}
			}

			DrawModelImportPopup();
		}

		ImGui::End();
		ImGui::PopStyleVar();
	}

	void EditorViewport::OpenModelImportDialog(const std::string& filePath)
	{
		namespace fs = std::filesystem;

		std::error_code ec{};
		fs::path abs = fs::weakly_canonical(filePath, ec);
		if (ec)
			abs = fs::path(filePath);

		fs::path root = AssetManager::Instance().getAssetPath();
		std::string id = fs::relative(abs, root, ec).generic_string();
		if (ec || id.empty())
			id = abs.filename().generic_string();

		std::string ext;
		if (size_t dot = id.find_last_of('.'); dot != std::string::npos)
		{
			ext = id.substr(dot + 1);
			for (char& ch : ext)
				ch = (char)std::tolower((unsigned char)ch);
		}

		ModelImportDialogState dlg{};
		dlg.requestOpen = true;
		dlg.open = true;
		dlg.absolutePath = abs.generic_string();
		dlg.assetId = std::move(id);
		dlg.extension = std::move(ext);
		dlg.instantiateInScene = true;
		dlg.preset = ModelImportPreset::Full;

		auto& opts = dlg.options;
		opts.drawMode = DrawMode::TRIANGLES;
		opts.buildMeshes = true;
		opts.loadMaterials = true;
		opts.loadSkinning = true;
		opts.loadAnimations = true;
		opts.vertexLayout.reset();
		opts.triangulate = false;
		opts.joinIdenticalVertices = false;
		opts.improveCacheLocality = false;
		opts.genUVIfMissing = false;
		opts.generateNormals = false;
		opts.generateTangents = true;
		opts.loadTangents = true;

		ApplyPreset(opts, dlg.preset);

		m_ModelImportDialog = std::move(dlg);
	}

	void EditorViewport::EnsureModelReady(const ModelImportDialogState& dialog)
	{
		using std::shared_ptr;

		const std::string& filePathAbs = dialog.absolutePath;
		const std::string& id = dialog.assetId;
		const ModelImportOptions& opts = dialog.options;

		auto isRenderable = [](const shared_ptr<Model>& m) -> bool {
			if (!m) return false;
			const auto& li = m->GetLoadedInfo();
			if (li.meshes.empty()) return false;
			for (const auto& mi : li.meshes)
				if (mi.mesh) return true;
			return false;
			};

		bool loaded = AssetManager::Instance().isAssetLoaded(id);
		if (loaded) {
			auto mdl = AssetManager::Instance().getAsset<Model>(id);
			if (isRenderable(mdl))
				return;
		}

		AssetToLoad a{};
		a.id = id;
		a.path = filePathAbs;
		a.type = AssetType::MODEL;
		a.spec = opts;

		AssetManager::Instance().updateAsset(a);
	}

	void EditorViewport::DrawModelImportPopup()
	{
		if (!m_ModelImportDialog.open && !m_ModelImportDialog.requestOpen)
			return;

		const char* popupName = "Importer un modele##ModelImport";

		if (m_ModelImportDialog.requestOpen)
		{
			ImGui::OpenPopup(popupName);
			m_ModelImportDialog.requestOpen = false;
		}

		bool keepOpen = m_ModelImportDialog.open;

		if (ImGui::BeginPopupModal(popupName, &keepOpen,
			ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoSavedSettings))
		{
			auto& dlg = m_ModelImportDialog;
			auto& opts = dlg.options;

			ImGui::TextUnformatted("Importer un modele");
			ImGui::Separator();

			ImGui::TextDisabled("Fichier source");
			ImGui::TextWrapped("%s", dlg.absolutePath.c_str());

			ImGui::Spacing();

			static char assetIdBuf[256];
			if (dlg.assetId.size() >= sizeof(assetIdBuf))
				dlg.assetId.resize(sizeof(assetIdBuf) - 1);

			std::strncpy(assetIdBuf, dlg.assetId.c_str(), sizeof(assetIdBuf));
			assetIdBuf[sizeof(assetIdBuf) - 1] = '\0';

			if (ImGui::InputText("ID d'asset", assetIdBuf, sizeof(assetIdBuf)))
				dlg.assetId = assetIdBuf;
			Tooltip("Nom interne utilise par l'AssetManager pour referencer le modele.");

			ImGui::Separator();

			const char* presetLabel = GetPresetLabel(dlg.preset);
			if (ImGui::BeginCombo("Profil d'import", presetLabel))
			{
				for (int i = 0; i < 4; ++i)
				{
					ModelImportPreset presetCandidate = (ModelImportPreset)i;
					bool selected = (presetCandidate == dlg.preset);
					if (ImGui::Selectable(GetPresetLabel(presetCandidate), selected))
					{
						dlg.preset = presetCandidate;
						if (dlg.preset != ModelImportPreset::Custom)
							ApplyPreset(opts, dlg.preset);
					}
					if (selected)
						ImGui::SetItemDefaultFocus();
				}
				ImGui::EndCombo();
			}
			Tooltip("Choisissez un profil d'import predefini. Toute modification manuelle bascule en 'Personnalise'.");

			ImGui::Spacing();

			int currentDrawModeIdx = GetDrawModeIndex(opts.drawMode);
			const char* drawModeLabel = kDrawModes[currentDrawModeIdx].label;

			if (ImGui::BeginCombo("Mode d'affichage", drawModeLabel))
			{
				for (int i = 0; i < (int)kDrawModes.size(); ++i)
				{
					bool selected = (i == currentDrawModeIdx);
					if (ImGui::Selectable(kDrawModes[i].label, selected))
					{
						currentDrawModeIdx = i;
						opts.drawMode = kDrawModes[i].mode;
						dlg.preset = ModelImportPreset::Custom;
					}
					if (selected)
						ImGui::SetItemDefaultFocus();
				}
				ImGui::EndCombo();
			}
			Tooltip("Mode de rendu utilise par defaut pour les meshes du modele.");

			ImGui::Separator();

			if (ImGui::BeginTable("ImportOptions", 2, ImGuiTableFlags_SizingStretchSame))
			{
				ImGui::TableNextColumn();
				ImGui::TextDisabled("Ce qu'on charge");

				if (ImGui::Checkbox("Meshes", &opts.buildMeshes))
					dlg.preset = ModelImportPreset::Custom;
				Tooltip("Importer la geometrie des meshes du modele.");

				if (ImGui::Checkbox("Materiaux", &opts.loadMaterials))
					dlg.preset = ModelImportPreset::Custom;
				Tooltip("Importer les materiaux (textures, couleurs...) du modele.");

				if (ImGui::Checkbox("Skinning", &opts.loadSkinning))
					dlg.preset = ModelImportPreset::Custom;
				Tooltip("Importer les informations de skinning (bones, poids...).");

				if (ImGui::Checkbox("Animations", &opts.loadAnimations))
					dlg.preset = ModelImportPreset::Custom;
				Tooltip("Importer les animations contenues dans le fichier.");

				if (ImGui::Checkbox("Tangentes (depuis le fichier)", &opts.loadTangents))
					dlg.preset = ModelImportPreset::Custom;
				Tooltip("Utiliser les tangentes fournies par le fichier si disponibles.");

				ImGui::TableNextColumn();
				ImGui::TextDisabled("Ce qu'on genere");

				if (ImGui::Checkbox("Trianguler", &opts.triangulate))
					dlg.preset = ModelImportPreset::Custom;
				Tooltip("Forcer tous les polygones a etre triangules.");

				if (ImGui::Checkbox("Fusionner les sommets identiques", &opts.joinIdenticalVertices))
					dlg.preset = ModelImportPreset::Custom;
				Tooltip("Fusionne les sommets partageant les memes attributs (optimisation).");

				if (ImGui::Checkbox("Ameliorer la locality cache", &opts.improveCacheLocality))
					dlg.preset = ModelImportPreset::Custom;
				Tooltip("Reordonne les indices pour ameliorer l'utilisation du cache GPU.");

				if (ImGui::Checkbox("Generer UV si manquantes", &opts.genUVIfMissing))
					dlg.preset = ModelImportPreset::Custom;
				Tooltip("Genere des coordonnees UV si le fichier n'en fournit pas.");

				if (ImGui::Checkbox("Generer normales", &opts.generateNormals))
					dlg.preset = ModelImportPreset::Custom;
				Tooltip("Recalcule les normales (utile si absentes ou incorrectes).");

				if (ImGui::Checkbox("Generer tangentes", &opts.generateTangents))
					dlg.preset = ModelImportPreset::Custom;
				Tooltip("Genere les tangentes (necessaires pour certaines techniques de shading).");

				ImGui::EndTable();
			}

			if (opts.generateTangents && !opts.generateNormals && !opts.loadTangents)
			{
				ImGui::Spacing();
				ImGui::TextColored(ImVec4(1.0f, 0.9f, 0.2f, 1.0f),
					"Attention : generation des tangentes sans normales. "
					"Les normales seront generees automatiquement.");
			}

			ImGui::Separator();

			ImGui::TextDisabled("Resume :");

			const char* drawModeStr =
				(opts.drawMode == DrawMode::POINTS) ? "Points" :
				(opts.drawMode == DrawMode::LINES) ? "Lignes" :
				(opts.drawMode == DrawMode::TRIANGLES) ? "Triangles" : "Inconnu";

			ImGui::BulletText("Mode d'affichage : %s", drawModeStr);
			ImGui::BulletText("Meshes: %s, Materiaux: %s, Skinning: %s, Animations: %s",
				opts.buildMeshes ? "oui" : "non",
				opts.loadMaterials ? "oui" : "non",
				opts.loadSkinning ? "oui" : "non",
				opts.loadAnimations ? "oui" : "non");

			const char* normalStr =
				opts.generateNormals ? "generees" :
				(opts.loadTangents ? "fichier (normales supposees correctes)" : "non gerees");

			const char* tangentStr =
				opts.generateTangents ? "generees" :
				(opts.loadTangents ? "depuis fichier" : "aucune");

			ImGui::BulletText("Normales : %s", normalStr);
			ImGui::BulletText("Tangentes : %s", tangentStr);

			ImGui::Separator();

			ImGui::Checkbox("Instancier dans la scene apres import", &dlg.instantiateInScene);
			Tooltip("Si coche, un GameObject sera cree dans la scene active avec ce modele.");

			ImGui::Spacing();

			if (ImGui::Button("Importer", ImVec2(120.0f, 0.0f)))
			{
				if (opts.generateTangents && !opts.generateNormals && !opts.loadTangents)
					opts.generateNormals = true;

				if (opts.drawMode == DrawMode::POINTS)
				{
					opts.vertexLayout = {
						{ ShaderDataType::Vec3, "inPosition" },
						{ ShaderDataType::Vec4, "inColor" }
					};
				}

				EnsureModelReady(dlg);

				if (dlg.instantiateInScene && m_Context.sceneManager)
					m_Context.sceneManager->AddGameObject(dlg.absolutePath);

				keepOpen = false;
				ImGui::CloseCurrentPopup();
			}

			ImGui::SameLine();

			if (ImGui::Button("Annuler", ImVec2(120.0f, 0.0f)))
			{
				keepOpen = false;
				ImGui::CloseCurrentPopup();
			}

			ImGui::EndPopup();
		}

		m_ModelImportDialog.open = keepOpen;
	}
}
