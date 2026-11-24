#pragma once

#include <imgui.h>

#include <string_view>

namespace Quasar::Editor::UI
{
    inline constexpr ImVec2 PopupPadding{24.0f, 20.0f};

    float Scaled(float size);
    ImVec2 Scaled(ImVec2 size);

    void DrawWrappedText(const ImVec4& color, std::string_view text);
    void DrawText(std::string_view text);
    void DrawDisabledText(std::string_view text);
    void DrawScaledText(std::string_view text, float scale);
    void DrawSection(std::string_view title);
    void DrawSeparator();

    void AlignButtonsRight(int count, float buttonWidth);
    bool BeginMessagePopup(const char* name, float width);

    bool IsConfirmPressed();
    bool IsCancelPressed();
}
