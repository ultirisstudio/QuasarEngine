#include "EditorViewport.h"

#include <imgui/imgui.h>
#include <ImGuizmo.h>

#include "../Editor.h"

#include "../SceneManager.h"

#include <QuasarEngine/Core/Application.h>
#include <QuasarEngine/Entity/Components/TransformComponent.h>
#include <QuasarEngine/Entity/Components/HierarchyComponent.h>
#include <QuasarEngine/Tools/Math.h>
#include <QuasarEngine/Core/Input.h>

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
		spec.Attachments = { FramebufferTextureFormat::RGBA8, FramebufferTextureFormat::Depth };

		m_EditorFrameBuffer = Framebuffer::Create(spec);
	}

	void EditorViewport::Render(Scene& scene, EditorCamera& camera)
	{
		m_EditorFrameBuffer->Bind();

		//RenderCommand::Clear();
		//RenderCommand::ClearColor(glm::vec4(0.5f, 0.5f, .5f, 1.0f));

		//m_EditorFrameBuffer->ClearAttachment(1, -1);

		Renderer::BeginScene(scene);

		Renderer::Render(camera);
		Renderer::RenderSkybox(camera);

		/*auto [mx, my] = ImGui::GetMousePos();
		mx -= m_EditorViewportBounds[0].x;
		my -= m_EditorViewportBounds[0].y;
		glm::vec2 editorViewportSize = m_EditorViewportBounds[1] - m_EditorViewportBounds[0];
		my = editorViewportSize.y - my;
		int mouseX = (int)mx;
		int mouseY = (int)my - m_WindowTitleBarSize[1];

		if (mouseX >= 0 && mouseY >= 0 && mouseX < editorViewportSize.x && mouseY < editorViewportSize.y)
		{
			int pixelData = m_EditorFrameBuffer->ReadPixel(1, mouseX, mouseY);

			m_HoveredEntity = scene.GetEntityByUUID(pixelData);
		}

		Renderer::RenderSkybox(camera);*/

		Renderer::EndScene();

		m_EditorFrameBuffer->Unbind();
	}

	void EditorViewport::Update(EditorCamera& camera)
	{
		if (m_EditorViewportSize != *((glm::vec2*)&m_ViewportPanelSize))
		{
			camera.OnResize(m_ViewportPanelSize.x, m_ViewportPanelSize.y);
			m_EditorFrameBuffer->Resize((uint32_t)m_ViewportPanelSize.x, (uint32_t)m_ViewportPanelSize.y);
			m_EditorViewportSize = { m_ViewportPanelSize.x, m_ViewportPanelSize.y };
		}

		if (m_ViewportHovered)
			camera.m_CameraFocus = true;
		else
			camera.m_CameraFocus = false;

		if (Input::IsKeyPressed(Key::E))
			m_GizmoType = -1;

		if (Input::IsKeyPressed(Key::R))
			m_GizmoType = ImGuizmo::OPERATION::TRANSLATE;

		if (Input::IsKeyPressed(Key::T))
			m_GizmoType = ImGuizmo::OPERATION::ROTATE;

		if (Input::IsKeyPressed(Key::Y))
			m_GizmoType = ImGuizmo::OPERATION::SCALE;
	}

    void EditorViewport::OnImGuiRender(EditorCamera& camera, SceneManager& sceneManager, SceneHierarchy& sceneHierarchy)
    {
        ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2{ 0, 0 });
        ImGui::Begin("Editor");

        m_ViewportFocused = ImGui::IsWindowFocused();
        m_ViewportHovered = ImGui::IsWindowHovered();

        auto windowSize = ImGui::GetWindowSize();
        auto viewportOffset = ImGui::GetCursorPos();
        m_ViewportPanelSize = ImGui::GetContentRegionAvail();

        if (m_EditorFrameBuffer)
        {
            void* handle = m_EditorFrameBuffer->GetColorAttachment(0);
            if (handle)
            {
                ImGui::Image((ImTextureID)handle, ImVec2{ m_EditorViewportSize.x, m_EditorViewportSize.y }, ImVec2{ 0, 0 }, ImVec2{ 1, 1 });
            }
        }

        ImVec2 minBound = ImGui::GetWindowPos();
        minBound.x += viewportOffset.x;
        minBound.y += viewportOffset.y;
        ImVec2 maxBound = { minBound.x + windowSize.x, minBound.y + windowSize.y };
        m_EditorViewportBounds[0] = { minBound.x, minBound.y };
        m_EditorViewportBounds[1] = { maxBound.x, maxBound.y };

        glm::mat4 cameraProjection = camera.getProjectionMatrix();
        glm::mat4 cameraView = camera.getViewMatrix();
        const float* view = glm::value_ptr(cameraView);
        const float* proj = glm::value_ptr(cameraProjection);

        ImGuizmo::SetOrthographic(false);
        ImGuizmo::SetDrawlist();
        ImGuizmo::SetRect(ImGui::GetWindowPos().x, ImGui::GetWindowPos().y,
            (float)ImGui::GetWindowWidth(), (float)ImGui::GetWindowHeight());

        if (sceneHierarchy.m_SelectedEntity && m_GizmoType != -1)
        {
            auto& tc = sceneHierarchy.m_SelectedEntity.GetComponent<TransformComponent>();
            glm::mat4 transform = tc.GetGlobalTransform();

            ImGuizmo::Manipulate(view, proj, (ImGuizmo::OPERATION)m_GizmoType, ImGuizmo::LOCAL, glm::value_ptr(transform));

            if (ImGuizmo::IsUsing())
            {
                if (ImGuizmo::IsOver())
                    camera.useGuizmo();
                else
                    camera.unuseGuizmo();

                UUID parentID = sceneHierarchy.m_SelectedEntity.GetComponent<HierarchyComponent>().m_Parent;
                glm::mat4 finalTransform = transform;

                while (parentID != UUID::Null())
                {
                    std::optional<Entity> parent = Renderer::m_SceneData.m_Scene->GetEntityByUUID(parentID);
                    if (parent.has_value())
                    {
                        glm::mat4 parentTransform = parent.value().GetComponent<TransformComponent>().GetLocalTransform();
                        finalTransform = finalTransform * glm::inverse(parentTransform); // Attention à l'ordre des multiplications
                        parentID = parent.value().GetComponent<HierarchyComponent>().m_Parent;
                    }
                    else
                    {
                        break;
                    }
                }

                glm::vec3 skew, translation, scale;
                glm::vec4 perspective;
                glm::quat rotationQuat;
                bool success = glm::decompose(finalTransform, scale, rotationQuat, translation, skew, perspective);

                if (success)
                {
                    tc.Position = translation;
                    tc.Scale = scale;

                    glm::vec3 euler = glm::eulerAngles(rotationQuat);

                    euler = glm::mod(euler + glm::pi<float>(), glm::two_pi<float>()) - glm::pi<float>();

                    tc.Rotation = euler;
                }
                else
                {
                    
                }

            }
            else
            {
                camera.unuseGuizmo();
            }
        }

        if (ImGui::BeginDragDropTarget())
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
                {
                    sceneManager.AddGameObject(filePath);
                }
                else if (fileExtension == "scene")
                {
                    sceneManager.LoadScene(filePath);
                }
            }
            ImGui::EndDragDropTarget();
        }

        ImGui::End();
        ImGui::PopStyleVar();

        // Infos debug (optionnel)
        /*
        ImGui::Begin("Editor infos");
        ImGui::Text("Hovered entity: %s", m_HoveredEntity ? m_HoveredEntity.GetName() : "none");
        ImGui::End();
        */
    }

}