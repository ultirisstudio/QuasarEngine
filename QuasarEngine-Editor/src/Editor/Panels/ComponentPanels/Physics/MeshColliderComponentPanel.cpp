#include "MeshColliderComponentPanel.h"

#include <imgui/imgui.h>

#include <QuasarEngine/Entity/Entity.h>
#include <QuasarEngine/Entity/Components/Physics/MeshColliderComponent.h>

namespace QuasarEngine
{
	void MeshColliderComponentPanel::Render(Entity entity)
	{
		if (entity.HasComponent<MeshColliderComponent>())
		{
			auto& cc = entity.GetComponent<MeshColliderComponent>();

			if (ImGui::TreeNodeEx("Mesh Collider", ImGuiTreeNodeFlags_DefaultOpen, "Mesh Collider"))
			{
				if (ImGui::BeginPopupContextItem())
				{
					if (ImGui::MenuItem("Delete Component"))
					{
						entity.RemoveComponent<MeshColliderComponent>();
					}
					ImGui::EndPopup();
				}

				ImGui::Checkbox("Is Convex", &cc.m_IsConvex);

				if (ImGui::Button("Generate collider"))
				{
					cc.Generate();
				}

				ImGui::TreePop();
			}
		}
	}
}