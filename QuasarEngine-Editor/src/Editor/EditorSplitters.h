#pragma once

#include <imgui/imgui.h>
#include <algorithm>

namespace QuasarEngine
{
    inline void SplitHorizontal(const char* id,
        float totalWidth,
        float& ratio,
        float minRatio = 0.2f,
        float maxRatio = 0.8f)
    {
        ImGui::SameLine(0.0f, 0.0f);
        ImGui::InvisibleButton(id, ImVec2(8.0f, ImGui::GetContentRegionAvail().y));
        if (ImGui::IsItemActive())
        {
            ratio += ImGui::GetIO().MouseDelta.x / totalWidth;
            ratio = std::clamp(ratio, minRatio, maxRatio);
        }
    }

    inline void SplitVertical(const char* id,
        float totalHeight,
        float& ratio,
        float minRatio = 0.25f,
        float maxRatio = 0.85f)
    {
        ImGui::InvisibleButton(id, ImVec2(-1.0f, 8.0f));
        if (ImGui::IsItemActive())
        {
            ratio += ImGui::GetIO().MouseDelta.y / totalHeight;
            ratio = std::clamp(ratio, minRatio, maxRatio);
        }
    }
}