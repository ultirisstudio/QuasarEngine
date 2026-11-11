#include "panels/ConsolePanel.h"

#include <imgui.h>
#include <spdlog/details/os.h>
#include <spdlog/fmt/chrono.h>
#include <spdlog/fmt/fmt.h>

#include <algorithm>
#include <ctime>
#include <memory>
#include <string_view>
#include <utility>

namespace Quasar::Editor
{
    namespace
    {
        constexpr int TimeColumn = 0;
        constexpr int LevelColumn = 1;
        constexpr int LoggerColumn = 2;
        constexpr int TextColumn = 3;
        constexpr int ColumnCount = 4;

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

        std::string FormatTime(std::chrono::system_clock::time_point time)
        {
            const std::tm localTime = spdlog::details::os::localtime(std::chrono::system_clock::to_time_t(time));
            return fmt::format("{:%H:%M:%S}", localTime);
        }

        std::string_view GetFirstLine(std::string_view text) { return text.substr(0, text.find('\n')); }
    }

    ConsolePanel::ConsolePanel(std::shared_ptr<MemoryLogSink> sink) : m_Sink(std::move(sink)) {}

    void ConsolePanel::OnImGuiRender()
    {
        if (ImGui::Begin("Console") && m_Sink != nullptr)
        {
            DrawMessages(m_Sink->GetMessages());
        }
        ImGui::End();
    }

    bool ConsolePanel::IsExpanded(const LogMessage& message) const
    {
        return std::ranges::any_of(m_Expanded, [&message](const MessageKey& key) { return IsSameMessage(message, key); });
    }

    std::string ConsolePanel::FormatMessage(const LogMessage& message)
    {
        return fmt::format("[{}] [{}] {}: {}", FormatTime(message.Time), GetLevelName(message.Level), message.LoggerName, message.Text);
    }

    bool ConsolePanel::IsSameMessage(const LogMessage& message, const MessageKey& key)
    {
        return message.Time == key.Time && message.LoggerName == key.LoggerName && message.Text == key.Text;
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
            m_Expanded.push_back({.Time = message.Time, .LoggerName = message.LoggerName, .Text = message.Text});
        }
    }

    void ConsolePanel::DrawMessages(const std::vector<LogMessage>& messages)
    {
        constexpr ImGuiTableFlags flags = ImGuiTableFlags_ScrollY | ImGuiTableFlags_RowBg | ImGuiTableFlags_BordersInnerV | ImGuiTableFlags_Resizable;
        if (!ImGui::BeginTable("Messages", ColumnCount, flags))
        {
            return;
        }

        ImGui::TableSetupColumn("Time", ImGuiTableColumnFlags_WidthFixed);
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

        ImGui::TableSetColumnIndex(LevelColumn);
        ImGui::TextUnformatted(GetLevelName(message.Level));
        ImGui::TableSetColumnIndex(LoggerColumn);
        ImGui::TextUnformatted(message.LoggerName.c_str());

        // The time is the clickable part of the row, and covers the whole row
        ImGui::TableSetColumnIndex(TimeColumn);
        const std::string time = FormatTime(message.Time);
        if (ImGui::Selectable(time.c_str(), false, ImGuiSelectableFlags_SpanAllColumns, ImVec2(0.0f, rowHeight)))
        {
            ToggleExpanded(message);
        }
        if (ImGui::BeginPopupContextItem("MessageMenu"))
        {
            if (ImGui::MenuItem("Copy"))
            {
                ImGui::SetClipboardText(FormatMessage(message).c_str());
            }
            ImGui::EndPopup();
        }

        ImGui::PopID();
    }
}
