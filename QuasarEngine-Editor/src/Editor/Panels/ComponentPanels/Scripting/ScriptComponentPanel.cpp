#include "ScriptComponentPanel.h"

#include <imgui/imgui.h>
#include <imgui/imgui_internal.h>

#include <QuasarEngine/Entity/Entity.h>
#include <QuasarEngine/Entity/Components/Scripting/ScriptComponent.h>

namespace QuasarEngine
{
	void ScriptComponentPanel::Render(Entity entity)
	{
		if (entity.HasComponent<ScriptComponent>())
		{
			auto& sc = entity.GetComponent<ScriptComponent>();

			if (ImGui::TreeNodeEx("Script", ImGuiTreeNodeFlags_DefaultOpen, "Script"))
			{
				char buffer[256];
				memset(buffer, 0, sizeof(buffer));
				strncpy(buffer, sc.scriptPath.c_str(), sizeof(buffer) - 1);

				if (ImGui::InputText("Script Path", buffer, sizeof(buffer)))
				{
					sc.scriptPath = buffer;
				}

				if (ImGui::Button("Reload Script"))
				{
					sc.Initialize();
				}

				ImGui::TreePop();
			}
		}
	}
}