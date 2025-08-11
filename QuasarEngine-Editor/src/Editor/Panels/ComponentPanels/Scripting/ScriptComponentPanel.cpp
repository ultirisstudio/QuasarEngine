#include "ScriptComponentPanel.h"

#include <filesystem>

#include <imgui/imgui.h>
#include <imgui/imgui_internal.h>

#include <QuasarEngine/Entity/Entity.h>
#include <QuasarEngine/Entity/Components/Scripting/ScriptComponent.h>

namespace QuasarEngine
{
	ScriptComponentPanel::ScriptComponentPanel(const std::string& projectPath)
		: m_ProjectPath(projectPath)
	{
		m_LocalBuffer[0] = '\0';
	}

	void ScriptComponentPanel::Render(Entity entity)
	{
		if (entity.HasComponent<ScriptComponent>())
		{
			auto& sc = entity.GetComponent<ScriptComponent>();

			if (m_LastEntityID != entity.GetUUID() || sc.scriptPath != m_LastFullPath)
			{
				try
				{
					std::filesystem::path fullPath(sc.scriptPath);
					std::filesystem::path relativePath = std::filesystem::relative(fullPath, m_ProjectPath);

					std::strncpy(m_LocalBuffer, relativePath.string().c_str(), sizeof(m_LocalBuffer) - 1);
					m_LocalBuffer[sizeof(m_LocalBuffer) - 1] = '\0';
				}
				catch (...)
				{
					std::strncpy(m_LocalBuffer, sc.scriptPath.c_str(), sizeof(m_LocalBuffer) - 1);
					m_LocalBuffer[sizeof(m_LocalBuffer) - 1] = '\0';
				}

				m_LastEntityID = entity.GetUUID();
				m_LastFullPath = sc.scriptPath;
			}

			if (ImGui::TreeNodeEx("Script", ImGuiTreeNodeFlags_DefaultOpen, "Script"))
			{
				if (ImGui::InputText("Script Path", m_LocalBuffer, sizeof(m_LocalBuffer)))
				{
					sc.scriptPath = (std::filesystem::path(m_ProjectPath) / m_LocalBuffer).string();
					m_LastFullPath = sc.scriptPath;
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
