#include "MeshComponentPanel.h"

#include <imgui/imgui.h>

#include <QuasarEngine/Entity/Entity.h>
#include <QuasarEngine/Entity/Components/MeshComponent.h>

namespace QuasarEngine
{
	void MeshComponentPanel::Render(Entity entity)
	{
		if (entity.HasComponent<MeshComponent>())
		{
			auto& mc = entity.GetComponent<MeshComponent>();

			if (ImGui::TreeNodeEx("Mesh", ImGuiTreeNodeFlags_DefaultOpen, "Mesh"))
			{
				ImGui::Text("Mesh: "); ImGui::SameLine();
				ImGui::Text(mc.GetName().c_str());

				ImGui::TreePop();
			}
		}
	}
}
