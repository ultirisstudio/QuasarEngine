#include "CameraComponentPanel.h"

#include <imgui/imgui.h>
#include <QuasarEngine/Entity/Entity.h>
#include <QuasarEngine/Entity/Components/CameraComponent.h>
#include <QuasarEngine/Scene/Camera.h>
#include <QuasarEngine/Renderer/Renderer.h>

namespace QuasarEngine
{
	static constexpr const char* CameraTypeLabel(QuasarEngine::CameraType t) {
		switch (t) {
		case QuasarEngine::CameraType::Perspective:  return "Perspective";
		case QuasarEngine::CameraType::Orthographic: return "Orthographic";
		default: return "Unknown";
		}
	}

	void CameraComponentPanel::Render(Entity entity)
	{
		if (!entity || !entity.IsValid())
			return;

		if (!entity.HasComponent<CameraComponent>())
			return;

		auto& cc = entity.GetComponent<CameraComponent>();

		if (ImGui::TreeNodeEx("Camera", ImGuiTreeNodeFlags_DefaultOpen, "Camera"))
		{
			ImGui::Text("Camera parameters: ");

			ImGui::Separator();

			if (cc.Primary) ImGui::Text("Is Primary");
			else
			{
				if (ImGui::Button("Set to primary"))
				{
					std::optional<Entity> primaryCameraEntity = Renderer::Instance().m_SceneData.m_Scene->GetPrimaryCameraEntity();
					if (primaryCameraEntity.has_value())
					{
						if (&primaryCameraEntity.value().GetComponent<CameraComponent>() != &cc)
						{
							if (primaryCameraEntity.value().GetComponent<CameraComponent>().Primary == true)
							{
								primaryCameraEntity.value().GetComponent<CameraComponent>().Primary = false;
								cc.Primary = true;
							}
						}
					}
					else
					{
						cc.Primary = true;
					}
				}
			}

			const char* items[] = { "Perspective", "Orthographic" };
			const char* current_item = CameraTypeLabel(cc.Type);

			if (ImGui::BeginCombo("##combocameratype", current_item))
			{
				for (int n = 0; n < 2; n++)
				{
					const auto type = (n == 0) ? QuasarEngine::CameraType::Perspective : QuasarEngine::CameraType::Orthographic;

					bool is_selected = (cc.Type == type);

					if (ImGui::Selectable(items[n], is_selected))
					{
						cc.SetType(type);
					}
					if (is_selected)
					{
						ImGui::SetItemDefaultFocus();
					}
				}
				ImGui::EndCombo();
			}

			ImGui::Text("FOV: "); ImGui::SameLine();
			float fov = cc.FovDeg;
			if (ImGui::DragFloat("##FOV", &fov, 0.1f, 0.0f, 180.0f, "%.1f"))
			{
				cc.SetFov(fov);
			}

			if (ImGui::BeginPopupContextItem())
			{
				if (ImGui::MenuItem("Delete Component"))
				{
					entity.RemoveComponent<CameraComponent>();
				}
				ImGui::EndPopup();
			}
			ImGui::TreePop();
		}
	}
}