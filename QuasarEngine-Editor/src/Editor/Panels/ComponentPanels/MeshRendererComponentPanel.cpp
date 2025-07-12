#include "MeshRendererComponentPanel.h"

#include <imgui/imgui.h>

#include <QuasarEngine/Entity/Entity.h>
#include <QuasarEngine/Entity/Components/MeshRendererComponent.h>

namespace QuasarEngine
{
	void MeshRendererComponentPanel::Render(Entity entity)
	{
		if (entity.HasComponent<MeshRendererComponent>())
		{
			auto& mrc = entity.GetComponent<MeshRendererComponent>();

			if (ImGui::TreeNodeEx("Mesh Renderer", ImGuiTreeNodeFlags_DefaultOpen, "Mesh Renderer"))
			{
				ImGui::Text("Render: "); ImGui::SameLine();
				ImGui::Checkbox("##Render", &mrc.m_Rendered);

				ImGui::TreePop();
			}
		}
	}
}
