#include "panels/ConsolePanel.h"

#include <quasar/log/Log.h>

#include <imgui.h>
#include <spdlog/details/os.h>
#include <spdlog/fmt/chrono.h>
#include <spdlog/fmt/fmt.h>

#include <algorithm>
#include <array>
#include <cstddef>
#include <cstdint>
#include <ctime>
#include <memory>
#include <string>
#include <string_view>

namespace Quasar::Editor
{
    namespace
    {
        constexpr int MarkColumn = 0;
        constexpr int TimeColumn = 1;
        constexpr int LevelColumn = 2;
        constexpr int LoggerColumn = 3;
        constexpr int TextColumn = 4;
        constexpr int ColumnCount = 5;

        constexpr float MarkScale = 0.4f;
        constexpr float MarkThickness = 1.5f;
        constexpr float CrossScale = 0.45f;

        constexpr ImVec4 Rgb(std::uint32_t hex)
        {
            constexpr float maxChannel = 255.0f;
            return {static_cast<float>((hex >> 16U) & 0xFFU) / maxChannel, static_cast<float>((hex >> 8U) & 0xFFU) / maxChannel,
                    static_cast<float>(hex & 0xFFU) / maxChannel, 1.0f};
        }

        constexpr ImVec4 DebugColor = Rgb(0x9DA1AA);
        constexpr ImVec4 WarningColor = Rgb(0xE8B04B);
        constexpr ImVec4 ErrorColor = Rgb(0xE5534B);

        constexpr std::array<ConsoleCategory, 3> Categories = {ConsoleCategory::Debug, ConsoleCategory::Warning, ConsoleCategory::Error};

        std::size_t ToIndex(ConsoleCategory category)
        {
            return static_cast<std::size_t>(category);
        }

        ConsoleCategory GetCategory(LogLevel level)
        {
            switch (level)
            {
                case LogLevel::Trace:
                case LogLevel::Debug:
                case LogLevel::Info:
                    return ConsoleCategory::Debug;
                case LogLevel::Warning:
                    return ConsoleCategory::Warning;
                case LogLevel::Error:
                case LogLevel::Critical:
                    return ConsoleCategory::Error;
            }
            return ConsoleCategory::Debug;
        }

        ImVec4 GetColor(ConsoleCategory category)
        {
            switch (category)
            {
                case ConsoleCategory::Debug:
                    return DebugColor;
                case ConsoleCategory::Warning:
                    return WarningColor;
                case ConsoleCategory::Error:
                    return ErrorColor;
            }
            return DebugColor;
        }

        const char* GetCategoryName(ConsoleCategory category)
        {
            switch (category)
            {
                case ConsoleCategory::Debug:
                    return "Debug";
                case ConsoleCategory::Warning:
                    return "Warnings";
                case ConsoleCategory::Error:
                    return "Errors";
            }
            return "Debug";
        }

        const char* GetFilterButtonId(ConsoleCategory category)
        {
            switch (category)
            {
                case ConsoleCategory::Debug:
                    return "##DebugFilter";
                case ConsoleCategory::Warning:
                    return "##WarningFilter";
                case ConsoleCategory::Error:
                    return "##ErrorFilter";
            }
            return "##DebugFilter";
        }

        const char* GetLevelName(LogLevel level)
        {
            switch (level)
            {
                case LogLevel::Trace:
                    return "Trace";
                case LogLevel::Debug:
                    return "Debug";
                case LogLevel::Info:
                    return "Info";
                case LogLevel::Warning:
                    return "Warning";
                case LogLevel::Error:
                    return "Error";
                case LogLevel::Critical:
                    return "Critical";
            }
            return "Unknown";
        }

        void DrawMark(ImDrawList* drawList, ImVec2 center, ConsoleCategory category)
        {
            const float radius = ImGui::GetTextLineHeight() * MarkScale;
            const ImU32 color = ImGui::GetColorU32(GetColor(category));
            switch (category)
            {
                case ConsoleCategory::Debug:
                    drawList->AddCircle(center, radius - (MarkThickness / 2.0f), color, 0, MarkThickness);
                    break;
                case ConsoleCategory::Warning:
                    drawList->AddTriangleFilled(ImVec2(center.x, center.y - radius), ImVec2(center.x + radius, center.y + radius),
                                                ImVec2(center.x - radius, center.y + radius), color);
                    break;
                case ConsoleCategory::Error:
                {
                    drawList->AddCircleFilled(center, radius, color);
                    const ImU32 crossColor = ImGui::GetColorU32(ImGuiCol_WindowBg);
                    const float arm = radius * CrossScale;
                    drawList->AddLine(ImVec2(center.x - arm, center.y - arm), ImVec2(center.x + arm, center.y + arm), crossColor, MarkThickness);
                    drawList->AddLine(ImVec2(center.x - arm, center.y + arm), ImVec2(center.x + arm, center.y - arm), crossColor, MarkThickness);
                    break;
                }
            }
        }

        std::string FormatTime(std::chrono::system_clock::time_point time)
        {
            const std::tm localTime = spdlog::details::os::localtime(std::chrono::system_clock::to_time_t(time));
            return fmt::format("{:%H:%M:%S}", localTime);
        }

        float GetTimeWidth()
        {
            float digitWidth = 0.0f;
            for (char digit = '0'; digit <= '9'; ++digit)
            {
                const std::array<char, 2> text = {digit, '\0'};
                digitWidth = std::max(digitWidth, ImGui::CalcTextSize(text.data()).x);
            }
            return (digitWidth * 6.0f) + (ImGui::CalcTextSize(":").x * 2.0f);
        }

        std::string FormatOneMessage(const LogMessage& message)
        {
            return fmt::format("[{}] [{}] {}: {}", FormatTime(message.Time), GetLevelName(message.Level), message.LoggerName, message.Text);
        }

        std::string_view GetFirstLine(std::string_view text)
        {
            return text.substr(0, text.find('\n'));
        }

        ConsoleCounts CountMessages(const std::vector<LogMessage>& messages)
        {
            ConsoleCounts counts;
            for (const LogMessage& message : messages)
            {
                switch (GetCategory(message.Level))
                {
                    case ConsoleCategory::Debug:
                        ++counts.Debug;
                        break;
                    case ConsoleCategory::Warning:
                        ++counts.Warning;
                        break;
                    case ConsoleCategory::Error:
                        ++counts.Error;
                        break;
                }
            }
            return counts;
        }

        std::size_t GetCount(const ConsoleCounts& counts, ConsoleCategory category)
        {
            switch (category)
            {
                case ConsoleCategory::Debug:
                    return counts.Debug;
                case ConsoleCategory::Warning:
                    return counts.Warning;
                case ConsoleCategory::Error:
                    return counts.Error;
            }
            return 0;
        }

        std::string FormatMessages(const std::vector<LogMessage>& messages)
        {
            std::string text;
            for (const LogMessage& message : messages)
            {
                if (!text.empty())
                {
                    text += '\n';
                }
                text += FormatOneMessage(message);
            }
            return text;
        }

        ImVec2 GetFilterButtonSize(std::size_t count)
        {
            const ImGuiStyle& style = ImGui::GetStyle();
            const float textWidth = ImGui::CalcTextSize(std::to_string(count).c_str()).x;
            return {(style.FramePadding.x * 2.0f) + ImGui::GetTextLineHeight() + style.ItemInnerSpacing.x + textWidth, ImGui::GetFrameHeight()};
        }
    }

    ConsolePanel::ConsolePanel(EditorContext& context) : EditorPanel("Console", context), m_Sink(Log::GetMemorySink()) {}

    void ConsolePanel::OnImGuiRender()
    {
        const std::vector<LogMessage> messages = GetMessages();
        const std::vector<LogMessage> shown = FilterShown(messages);
        DrawToolbar(CountMessages(messages), shown);
        DrawMessages(shown);
    }

    bool ConsolePanel::IsShown(ConsoleCategory category) const
    {
        return m_Shown.at(ToIndex(category));
    }

    bool ConsolePanel::IsExpanded(const LogMessage& message) const
    {
        return std::ranges::any_of(m_Expanded, [&message](const MessageKey& key) { return IsSameMessage(message, key); });
    }

    std::string ConsolePanel::FormatMessage(const LogMessage& message)
    {
        return FormatOneMessage(message);
    }

    ConsolePanel::MessageKey ConsolePanel::GetKey(const LogMessage& message)
    {
        return {.Time = message.Time, .LoggerName = message.LoggerName, .Text = message.Text};
    }

    bool ConsolePanel::IsSameMessage(const LogMessage& message, const MessageKey& key)
    {
        return message.Time == key.Time && message.LoggerName == key.LoggerName && message.Text == key.Text;
    }

    std::vector<LogMessage> ConsolePanel::GetMessages() const
    {
        if (m_Sink == nullptr)
        {
            return {};
        }

        std::vector<LogMessage> messages = m_Sink->GetMessages();
        if (!m_LastCleared)
        {
            return messages;
        }

        const auto lastCleared =
            std::find_if(messages.rbegin(), messages.rend(), [this](const LogMessage& message) { return IsSameMessage(message, *m_LastCleared); });
        if (lastCleared != messages.rend())
        {
            const auto clearedEnd = lastCleared.base();
            const auto keptEnd =
                std::remove_if(messages.begin(), clearedEnd, [](const LogMessage& message) { return GetCategory(message.Level) != ConsoleCategory::Error; });
            messages.erase(keptEnd, clearedEnd);
        }
        return messages;
    }

    std::vector<LogMessage> ConsolePanel::FilterShown(std::vector<LogMessage> messages) const
    {
        std::erase_if(messages, [this](const LogMessage& message) { return !IsShown(GetCategory(message.Level)); });
        return messages;
    }

    void ConsolePanel::Clear()
    {
        if (m_Sink == nullptr)
        {
            return;
        }

        const std::vector<LogMessage> messages = m_Sink->GetMessages();
        if (!messages.empty())
        {
            m_LastCleared = GetKey(messages.back());
        }
    }

    void ConsolePanel::ToggleExpanded(const LogMessage& message)
    {
        const auto expanded = std::ranges::find_if(m_Expanded, [&message](const MessageKey& key) { return IsSameMessage(message, key); });
        if (expanded != m_Expanded.end())
        {
            m_Expanded.erase(expanded);
        }
        else
        {
            m_Expanded.push_back(GetKey(message));
        }
    }

    void ConsolePanel::DrawToolbar(const ConsoleCounts& counts, const std::vector<LogMessage>& shown)
    {
        if (ImGui::Button("Clear"))
        {
            Clear();
        }
        ImGui::SameLine();
        if (ImGui::Button("Copy all"))
        {
            ImGui::SetClipboardText(FormatMessages(shown).c_str());
        }

        float filtersWidth = 0.0f;
        for (const ConsoleCategory category : Categories)
        {
            filtersWidth += GetFilterButtonSize(GetCount(counts, category)).x;
        }
        filtersWidth += ImGui::GetStyle().ItemSpacing.x * static_cast<float>(Categories.size() - 1);

        ImGui::SameLine();
        const float offset = ImGui::GetContentRegionAvail().x - filtersWidth;
        if (offset > 0.0f)
        {
            ImGui::SetCursorPosX(ImGui::GetCursorPosX() + offset);
        }

        for (const ConsoleCategory category : Categories)
        {
            if (category != Categories.front())
            {
                ImGui::SameLine();
            }
            DrawFilterButton(category, GetCount(counts, category));
        }

        ImGui::Separator();
    }

    void ConsolePanel::DrawFilterButton(ConsoleCategory category, std::size_t count)
    {
        const bool shown = IsShown(category);
        if (!shown)
        {
            ImGui::PushStyleVar(ImGuiStyleVar_Alpha, ImGui::GetStyle().Alpha * ImGui::GetStyle().DisabledAlpha);
        }

        const ImVec2 size = GetFilterButtonSize(count);
        const bool clicked = ImGui::Button(GetFilterButtonId(category), size);

        const ImGuiStyle& style = ImGui::GetStyle();
        const ImVec2 corner = ImGui::GetItemRectMin();
        const float lineHeight = ImGui::GetTextLineHeight();
        ImDrawList* drawList = ImGui::GetWindowDrawList();
        DrawMark(drawList, ImVec2(corner.x + style.FramePadding.x + (lineHeight / 2.0f), corner.y + (size.y / 2.0f)), category);
        drawList->AddText(ImVec2(corner.x + style.FramePadding.x + lineHeight + style.ItemInnerSpacing.x, corner.y + style.FramePadding.y),
                          ImGui::GetColorU32(ImGuiCol_Text), std::to_string(count).c_str());

        if (!shown)
        {
            ImGui::PopStyleVar();
        }

        if (ImGui::BeginItemTooltip())
        {
            ImGui::TextUnformatted(GetCategoryName(category));
            ImGui::EndTooltip();
        }

        if (clicked)
        {
            m_Shown.at(ToIndex(category)) = !shown;
        }
    }

    void ConsolePanel::DrawMessages(const std::vector<LogMessage>& messages)
    {
        constexpr ImGuiTableFlags flags = ImGuiTableFlags_ScrollY | ImGuiTableFlags_RowBg | ImGuiTableFlags_BordersInnerV | ImGuiTableFlags_Resizable;
        if (!ImGui::BeginTable("Messages", ColumnCount, flags))
        {
            return;
        }

        ImGui::TableSetupColumn("Mark", ImGuiTableColumnFlags_WidthFixed | ImGuiTableColumnFlags_NoResize, ImGui::GetTextLineHeight());
        ImGui::TableSetupColumn("Time", ImGuiTableColumnFlags_WidthFixed, GetTimeWidth());
        ImGui::TableSetupColumn("Level", ImGuiTableColumnFlags_WidthFixed);
        ImGui::TableSetupColumn("Logger", ImGuiTableColumnFlags_WidthFixed);
        ImGui::TableSetupColumn("Text", ImGuiTableColumnFlags_WidthStretch);

        int index = 0;
        for (const LogMessage& message : messages)
        {
            DrawMessage(index, message);
            ++index;
        }

        ImGui::EndTable();
    }

    void ConsolePanel::DrawMessage(int index, const LogMessage& message)
    {
        ImGui::TableNextRow();
        ImGui::PushID(index);

        const ConsoleCategory category = GetCategory(message.Level);
        const bool colored = category != ConsoleCategory::Debug;
        if (colored)
        {
            ImGui::PushStyleColor(ImGuiCol_Text, GetColor(category));
        }

        ImGui::TableSetColumnIndex(TextColumn);
        if (IsExpanded(message))
        {
            ImGui::PushTextWrapPos(0.0f);
            ImGui::TextUnformatted(message.Text.c_str());
            ImGui::PopTextWrapPos();
        }
        else
        {
            const std::string_view firstLine = GetFirstLine(message.Text);
            ImGui::TextUnformatted(firstLine.data(), std::to_address(firstLine.end()));
        }
        const float rowHeight = ImGui::GetItemRectSize().y;

        ImGui::TableSetColumnIndex(MarkColumn);
        const float lineHeight = ImGui::GetTextLineHeight();
        const ImVec2 corner = ImGui::GetCursorScreenPos();
        DrawMark(ImGui::GetWindowDrawList(), ImVec2(corner.x + (lineHeight / 2.0f), corner.y + (lineHeight / 2.0f)), category);
        ImGui::Dummy(ImVec2(lineHeight, lineHeight));

        ImGui::TableSetColumnIndex(LevelColumn);
        ImGui::TextUnformatted(GetLevelName(message.Level));
        ImGui::TableSetColumnIndex(LoggerColumn);
        ImGui::TextUnformatted(message.LoggerName.c_str());

        ImGui::TableSetColumnIndex(TimeColumn);
        const std::string time = FormatTime(message.Time);
        const bool clicked = ImGui::Selectable(time.c_str(), false, ImGuiSelectableFlags_SpanAllColumns, ImVec2(0.0f, rowHeight));

        if (colored)
        {
            ImGui::PopStyleColor();
        }

        if (clicked)
        {
            ToggleExpanded(message);
        }
        if (ImGui::BeginPopupContextItem("MessageMenu"))
        {
            if (ImGui::MenuItem("Copy"))
            {
                ImGui::SetClipboardText(FormatMessage(message).c_str());
            }

            if (ImGui::MenuItem("Copy all"))
            {
                ImGui::SetClipboardText(FormatMessages(FilterShown(GetMessages())).c_str());
            }
            ImGui::EndPopup();
        }

        ImGui::PopID();
    }
}
