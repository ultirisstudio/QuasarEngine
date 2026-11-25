#include "EditorUI.h"

#include "EditorText.h"

#include <quasar/localization/TextLayout.h>

#include <algorithm>
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
