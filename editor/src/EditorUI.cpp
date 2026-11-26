#include "EditorUI.h"

#include "EditorText.h"

#include <quasar/localization/TextLayout.h>

#include <imgui_internal.h>

#include <algorithm>
#include <array>
#include <cstddef>
#include <ranges>
#include <utility>

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

    std::string JoinList(std::span<const char* const> items)
    {
        const Localization& localization = GetLocalization();
        const bool rightToLeft = localization.GetDirection() == TextDirection::RightToLeft;
        std::string separator = rightToLeft ? " ," : ", ";
        if (localization.Contains(ListSeparatorKey))
        {
            separator = localization.Get(ListSeparatorKey);
        }
        std::string text;
        for (std::size_t index = 0; index < items.size(); ++index)
        {
            text += (index == 0 ? "" : separator) + std::string(items[rightToLeft ? items.size() - 1 - index : index]);
        }
        return text;
    }

    std::vector<std::string> WrapRightToLeft(std::string_view text, float width, const std::function<float(std::string_view)>& measure)
    {
        std::vector<std::string> lines;
        std::size_t start = 0;
        while (start <= text.size())
        {
            const std::size_t end = std::min(text.find('\n', start), text.size());
            std::vector<std::string_view> words;
            for (std::size_t word = start; word <= end;)
            {
                const std::size_t space = std::min(text.find(' ', word), end);
                words.push_back(text.substr(word, space - word));
                word = space + 1;
            }

            std::string line;
            for (const std::string_view word : std::views::reverse(words))
            {
                std::string candidate = line.empty() ? std::string(word) : std::string(word) + " " + line;
                if (!line.empty() && measure(candidate) > width)
                {
                    lines.push_back(std::move(line));
                    line = std::string(word);
                }
                else
                {
                    line = std::move(candidate);
                }
            }
            lines.push_back(std::move(line));
            start = end + 1;
        }
        return lines;
    }

    void DrawWrappedText(const ImVec4& color, std::string_view text)
    {
        ImGui::PushStyleColor(ImGuiCol_Text, color);
        if (GetLocalization().GetDirection() == TextDirection::RightToLeft && HasRightToLeftText(text))
        {
            const float width = ImGui::GetContentRegionAvail().x;
            const auto measure = [](std::string_view line) { return ImGui::CalcTextSize(line.data(), line.data() + line.size()).x; };
            for (const std::string& line : WrapRightToLeft(text, width, measure))
            {
                ImGui::SetCursorPosX(ImGui::GetCursorPosX() + std::max(0.0f, width - measure(line)));
                ImGui::TextUnformatted(line.c_str());
            }
        }
        else
        {
            ImGui::PushTextWrapPos(0.0f);
            ImGui::TextUnformatted(text.data(), text.data() + text.size());
            ImGui::PopTextWrapPos();
        }
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

    void DrawFolderIcon(ImDrawList* drawList, ImVec2 center, float width, ImU32 color)
    {
        const float height = width * 0.75f;
        const ImVec2 bodyMin{center.x - (width / 2.0f), center.y - (height / 2.0f) + (height * 0.15f)};
        const ImVec2 bodyMax{center.x + (width / 2.0f), center.y + (height / 2.0f)};
        const ImVec2 tabMin{bodyMin.x, bodyMin.y - (height * 0.15f)};
        const float rounding = Scaled(CardRounding);
        const ImVec2 tabMax{bodyMin.x + (width * 0.4f), bodyMin.y + rounding};
        drawList->AddRectFilled(tabMin, tabMax, color, rounding / 2.0f);
        drawList->AddRectFilled(bodyMin, bodyMax, color, rounding / 2.0f);
    }

    void DrawFileIcon(ImDrawList* drawList, ImVec2 center, float width, ImU32 color)
    {
        const float pageWidth = width * 0.7f;
        const float height = width * 0.9f;
        const ImVec2 min{center.x - (pageWidth / 2.0f), center.y - (height / 2.0f)};
        const ImVec2 max{center.x + (pageWidth / 2.0f), center.y + (height / 2.0f)};
        const float fold = pageWidth * 0.3f;
        const std::array<ImVec2, 5> outline{ImVec2{min.x, min.y}, ImVec2{max.x - fold, min.y}, ImVec2{max.x, min.y + fold}, ImVec2{max.x, max.y},
                                            ImVec2{min.x, max.y}};
        drawList->AddPolyline(outline.data(), static_cast<int>(outline.size()), color, ImDrawFlags_Closed, Scaled(CardBorder));
        drawList->AddTriangleFilled(ImVec2{max.x - fold, min.y}, ImVec2{max.x, min.y + fold}, ImVec2{max.x - fold, min.y + fold}, color);
    }

    bool DrawCard(const char* id, std::string_view label, ImVec2 size, CardIcon icon, ImU32 iconColor, bool selected)
    {
        ImGui::PushID(id);
        const bool clicked = ImGui::InvisibleButton("Card", size);
        const bool hovered = ImGui::IsItemHovered();
        ImGui::PopID();

        const ImVec2 min = ImGui::GetItemRectMin();
        const ImVec2 max = ImGui::GetItemRectMax();
        ImDrawList* drawList = ImGui::GetWindowDrawList();

        drawList->AddRectFilled(min, max, ImGui::GetColorU32(hovered ? ImGuiCol_FrameBgHovered : ImGuiCol_FrameBg), Scaled(CardRounding));
        if (selected)
        {
            drawList->AddRect(min, max, ImGui::GetColorU32(ImGuiCol_CheckMark), Scaled(CardRounding), 0, Scaled(CardBorder));
        }

        const float labelHeight = ImGui::GetTextLineHeightWithSpacing() * 1.5f;
        const ImVec2 iconCenter{(min.x + max.x) / 2.0f, min.y + ((size.y - labelHeight) / 2.0f)};
        switch (icon)
        {
            case CardIcon::Folder:
                DrawFolderIcon(drawList, iconCenter, size.x * 0.45f, iconColor);
                break;
            case CardIcon::File:
                DrawFileIcon(drawList, iconCenter, size.x * 0.45f, iconColor);
                break;
        }

        const char* begin = label.data();
        const char* end = label.data() + label.size();
        const ImVec2 labelSize = ImGui::CalcTextSize(begin, end);
        const float padding = ImGui::GetStyle().FramePadding.x;
        const float labelY = max.y - labelHeight + ((labelHeight - labelSize.y) / 2.0f);
        if (labelSize.x <= size.x - (padding * 2.0f))
        {
            drawList->AddText(ImVec2{(min.x + max.x - labelSize.x) / 2.0f, labelY}, ImGui::GetColorU32(ImGuiCol_Text), begin, end);
            return clicked;
        }

        ImGui::RenderTextEllipsis(drawList, ImVec2{min.x + padding, labelY}, ImVec2{max.x - padding, labelY + labelSize.y}, max.x - padding, begin, end,
                                  &labelSize);
        if (ImGui::BeginItemTooltip())
        {
            ImGui::TextUnformatted(begin, end);
            ImGui::EndTooltip();
        }
        return clicked;
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
