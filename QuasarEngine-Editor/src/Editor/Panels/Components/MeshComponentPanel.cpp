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

				ImGui::Text("Model Path: "); ImGui::SameLine();
				ImGui::Text(mc.GetModelPath().c_str());

				ImGui::Text("Node Path: "); ImGui::SameLine();
				ImGui::Text(mc.GetNodePath().c_str());

				ImGui::Text("Has Local Node Transform: "); ImGui::SameLine();
				ImGui::Text(mc.HasLocalNodeTransform() ? "Yes" : "No");

				ImGui::Text("Has Mesh: "); ImGui::SameLine();
				ImGui::Text(mc.HasMesh() ? "Yes" : "No");

				ImGui::Text("Vertices Count: "); ImGui::SameLine();
				ImGui::Text("%zu", mc.HasMesh() ? mc.GetMesh().GetVerticesCount() : 0);

				ImGui::Text("Indices Count: "); ImGui::SameLine();
				ImGui::Text("%zu", mc.HasMesh() ? mc.GetMesh().GetIndicesCount() : 0);

				ImGui::TreePop();
			}
		}
	}
}
