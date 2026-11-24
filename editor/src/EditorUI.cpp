#include "EditorUI.h"

#include <algorithm>

namespace Quasar::Editor::UI
{
    float Scaled(float size)
    {
        return size * ImGui::GetStyle().FontScaleDpi;
    }

    ImVec2 Scaled(ImVec2 size)
    {
        return {Scaled(size.x), Scaled(size.y)};
    }

    void DrawWrappedText(const ImVec4& color, std::string_view text)
    {
        ImGui::PushStyleColor(ImGuiCol_Text, color);
        ImGui::PushTextWrapPos(0.0f);
        ImGui::TextUnformatted(text.data(), text.data() + text.size());
        ImGui::PopTextWrapPos();
        ImGui::PopStyleColor();
    }

    void DrawText(std::string_view text)
    {
        DrawWrappedText(ImGui::GetStyleColorVec4(ImGuiCol_Text), text);
    }

    void DrawDisabledText(std::string_view text)
    {
        DrawWrappedText(ImGui::GetStyleColorVec4(ImGuiCol_TextDisabled), text);
    }

    void DrawScaledText(std::string_view text, float scale)
    {
        ImGui::PushFont(nullptr, ImGui::GetStyle().FontSizeBase * scale);
        ImGui::TextUnformatted(text.data(), text.data() + text.size());
        ImGui::PopFont();
    }

    void DrawSection(std::string_view title)
    {
        DrawDisabledText(title);
        ImGui::Spacing();
    }

    void DrawSeparator()
    {
        ImGui::Spacing();
        ImGui::Separator();
        ImGui::Spacing();
    }

    void AlignButtonsRight(int count, float buttonWidth)
    {
        const float width = (buttonWidth * static_cast<float>(count)) + (ImGui::GetStyle().ItemSpacing.x * static_cast<float>(count - 1));
        ImGui::SetCursorPosX(ImGui::GetCursorPosX() + std::max(0.0f, ImGui::GetContentRegionAvail().x - width));
    }

    bool BeginMessagePopup(const char* name, float width)
    {
        ImGui::SetNextWindowPos(ImGui::GetMainViewport()->GetCenter(), ImGuiCond_Appearing, ImVec2(0.5f, 0.5f));
        ImGui::SetNextWindowSize(ImVec2(width, 0.0f));
        ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, Scaled(PopupPadding));
        const bool open = ImGui::BeginPopupModal(name, nullptr, ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoSavedSettings);
        ImGui::PopStyleVar();
        return open;
    }

    bool IsConfirmPressed()
    {
        return ImGui::IsKeyPressed(ImGuiKey_Enter) || ImGui::IsKeyPressed(ImGuiKey_KeypadEnter);
    }

    bool IsCancelPressed()
    {
        return ImGui::IsKeyPressed(ImGuiKey_Escape);
    }
}
