#pragma once

#include <imgui.h>

#include <cstdint>
#include <functional>
#include <span>
#include <string>
#include <string_view>
#include <vector>

namespace Quasar::Editor::UI
{
    inline constexpr ImVec2 PopupPadding{24.0f, 20.0f};

    inline constexpr float CardRounding = 6.0f;
    inline constexpr float CardBorder = 2.0f;

    enum class CardIcon : std::uint8_t
    {
        Folder,
        File
    };

    float Scaled(float size);
    ImVec2 Scaled(ImVec2 size);

    void DrawWrappedText(const ImVec4& color, std::string_view text);

    inline constexpr std::string_view ListSeparatorKey = "list_separator";

    std::string JoinList(std::span<const char* const> items);

    std::vector<std::string> WrapRightToLeft(std::string_view text, float width, const std::function<float(std::string_view)>& measure);
    void DrawText(std::string_view text);
    void DrawDisabledText(std::string_view text);
    void DrawScaledText(std::string_view text, float scale);
    void DrawSection(std::string_view title);
    void DrawSeparator();

    void DrawFolderIcon(ImDrawList* drawList, ImVec2 center, float width, ImU32 color);
    void DrawFileIcon(ImDrawList* drawList, ImVec2 center, float width, ImU32 color);

    bool DrawCard(const char* id, std::string_view label, ImVec2 size, CardIcon icon, ImU32 iconColor, bool selected);

    void AlignButtonsRight(int count, float buttonWidth);
    bool BeginMessagePopup(const char* name, float width);

    bool IsConfirmPressed();
    bool IsCancelPressed();
}
