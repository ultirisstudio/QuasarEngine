#include "CapsuleColliderComponentPanel.h"

#include <imgui/imgui.h>

#include <QuasarEngine/Entity/Entity.h>
#include <QuasarEngine/Entity/Components/Physics/CapsuleColliderComponent.h>

namespace QuasarEngine
{
	void CapsuleColliderComponentPanel::Render(Entity entity)
	{
		if (entity.HasComponent<CapsuleColliderComponent>())
		{
			auto& cc = entity.GetComponent<CapsuleColliderComponent>();

			if (ImGui::TreeNodeEx("Capsule Collider", ImGuiTreeNodeFlags_DefaultOpen, "Capsule Collider"))
			{
				if (ImGui::BeginPopupContextItem())
				{
					if (ImGui::MenuItem("Delete Component"))
					{
						entity.RemoveComponent<CapsuleColliderComponent>();
					}
					ImGui::EndPopup();
				}

				ImGui::Text("Radius: ");
				if (ImGui::DragFloat("##Radius", &cc.m_Radius, 0.05f, 0.1f, 10.0f))
				{
					cc.UpdateColliderSize();
				}

				ImGui::Text("Height: ");
				if (ImGui::DragFloat("##Height", &cc.m_Height, 0.05f, 0.1f, 10.0f))
				{
					cc.UpdateColliderSize();
				}

				if (ImGui::TreeNodeEx("Collider material", ImGuiTreeNodeFlags_DefaultOpen, "Collider material"))
				{
					ImGui::Text("Mass: ");
					if (ImGui::DragFloat("##Mass", &cc.mass, 1.0f, 1.0f, 1000.0f))
					{
						cc.UpdateColliderMaterial();
					}

					ImGui::Text("Friction: ");
					if (ImGui::DragFloat("##Friction", &cc.friction, 0.05f, 0.1f, 10.0f))
					{
						cc.UpdateColliderMaterial();
					}

					ImGui::Text("Bounciness: ");
					if (ImGui::DragFloat("##Bounciness", &cc.bounciness, 0.05f, 0.1f, 1.0f))
					{
						cc.UpdateColliderMaterial();
					}

					ImGui::TreePop();
				}

				ImGui::TreePop();
			}
		}
	}
}