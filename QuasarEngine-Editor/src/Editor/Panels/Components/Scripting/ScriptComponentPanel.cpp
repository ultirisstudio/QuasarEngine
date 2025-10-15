#include "ScriptComponentPanel.h"

#include <filesystem>

#include <imgui/imgui.h>
#include <imgui/imgui_internal.h>

#include <glm/glm.hpp>

#include <QuasarEngine/Entity/Entity.h>
#include <QuasarEngine/Entity/Components/Scripting/ScriptComponent.h>
#include <QuasarEngine/Core/UUID.h>

namespace QuasarEngine
{
    ScriptComponentPanel::ScriptComponentPanel(const std::string& projectPath)
        : m_ProjectPath(projectPath), m_LastEntityID(0), m_LastFullPath("")
    {
        m_LocalBuffer[0] = '\0';
    }

    void ScriptComponentPanel::Render(Entity entity)
    {
        if (!entity || !entity.IsValid())
            return;

        if (!entity.HasComponent<ScriptComponent>())
            return;

        auto& sc = entity.GetComponent<ScriptComponent>();

        if (m_LastEntityID != entity.GetUUID() || sc.scriptPath != m_LastFullPath)
        {
            std::strncpy(m_LocalBuffer, sc.scriptPath.c_str(), sizeof(m_LocalBuffer) - 1);
            m_LocalBuffer[sizeof(m_LocalBuffer) - 1] = '\0';

            m_LastEntityID = entity.GetUUID();
            m_LastFullPath = sc.scriptPath;
        }

        if (ImGui::TreeNodeEx("Script", ImGuiTreeNodeFlags_DefaultOpen, "Script"))
        {
            if (ImGui::InputText("Script Path", m_LocalBuffer, sizeof(m_LocalBuffer)))
            {
                sc.scriptPath = m_LocalBuffer;
                m_LastFullPath = sc.scriptPath;
            }

            {
                std::string relativeInfo;
                try
                {
                    if (!m_ProjectPath.empty() && !sc.scriptPath.empty())
                    {
                        std::filesystem::path full(sc.scriptPath);
                        std::error_code ec{};
                        auto rel = std::filesystem::relative(full, m_ProjectPath, ec);
                        if (!ec) relativeInfo = rel.generic_string();
                    }
                }
                catch (...) {  }

                /*if (!relativeInfo.empty())
                {
                    ImGui::BeginDisabled(true);
                    ImGui::InputText("Project-relative (info)", relativeInfo.data(), relativeInfo.size() + 1);
                    ImGui::EndDisabled();
                }*/
            }

            if (ImGui::Button("Reload Script"))
            {
                sc.Initialize();
            }

            if (ImGui::TreeNodeEx("Exposed (public)", ImGuiTreeNodeFlags_DefaultOpen, "Exposed (public)"))
            {
                if (sc.publicTable.valid() && sc.environment.valid())
                {
                    for (auto& rv : sc.reflectedVars)
                    {
                        const char* label = rv.name.c_str();

                        switch (rv.type)
                        {
                        case ScriptComponent::VarType::Number:
                        {
                            double val = sc.publicTable[rv.name].get_or(0.0);
                            float fval = static_cast<float>(val);
                            if (ImGui::DragFloat(label, &fval, 0.1f))
                            {
                                sc.publicTable[rv.name] = static_cast<double>(fval);
                                sc.environment[rv.name] = static_cast<double>(fval);
                            }
                            break;
                        }
                        case ScriptComponent::VarType::String:
                        {
                            sol::optional<std::string> sval = sc.publicTable[rv.name];
                            char buf[256];
                            std::snprintf(buf, sizeof(buf), "%s", sval.value_or("").c_str());
                            if (ImGui::InputText(label, buf, sizeof(buf)))
                            {
                                std::string newVal(buf);
                                sc.publicTable[rv.name] = newVal;
                                sc.environment[rv.name] = newVal;
                            }
                            break;
                        }
                        case ScriptComponent::VarType::Boolean:
                        {
                            bool bval = sc.publicTable[rv.name].get_or(false);
                            if (ImGui::Checkbox(label, &bval))
                            {
                                sc.publicTable[rv.name] = bval;
                                sc.environment[rv.name] = bval;
                            }
                            break;
                        }
                        case ScriptComponent::VarType::Vec3:
                        {
                            glm::vec3 vval = sc.publicTable[rv.name].get_or(glm::vec3{ 0.0f, 0.0f, 0.0f });
                            float arr[3] = { vval.x, vval.y, vval.z };
                            if (ImGui::DragFloat3(label, arr, 0.1f))
                            {
                                glm::vec3 newV(arr[0], arr[1], arr[2]);
                                sc.publicTable[rv.name] = newV;
                                sc.environment[rv.name] = newV;
                            }
                            break;
                        }
                        }
                    }
                }
                else
                {
                    ImGui::TextDisabled("No 'public' table in this script or environment not initialized.");
                }
                ImGui::TreePop();
            }

            ImGui::TreePop();
        }
    }
}