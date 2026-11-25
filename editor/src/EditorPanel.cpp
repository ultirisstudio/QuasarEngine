#include "EditorPanel.h"

#include "EditorText.h"

#include <imgui.h>

#include <string>
#include <utility>

namespace Quasar::Editor
{
    EditorPanel::EditorPanel(std::string key, EditorContext& context, ImGuiWindowFlags flags) : m_Key(std::move(key)), m_Context(&context), m_Flags(flags) {}

    std::string EditorPanel::GetLabel() const
    {
        return TrLabel(m_Key);
    }

    void EditorPanel::Draw()
    {
        m_Focused = false;
        m_Hovered = false;
        if (!m_Open)
        {
            return;
        }

        if (m_InitialSize.x > 0.0f && m_InitialSize.y > 0.0f)
        {
            ImGui::SetNextWindowSize(m_InitialSize, ImGuiCond_FirstUseEver);
        }
        if (ImGui::Begin(GetLabel().c_str(), &m_Open, m_Flags))
        {
            m_Focused = ImGui::IsWindowFocused(ImGuiFocusedFlags_RootAndChildWindows);
            m_Hovered = ImGui::IsWindowHovered(ImGuiHoveredFlags_RootAndChildWindows);
            OnImGuiRender();
        }
        ImGui::End();
    }
}
