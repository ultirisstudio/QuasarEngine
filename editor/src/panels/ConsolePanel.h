#pragma once

#include "EditorPanel.h"

#include <quasar/log/MemoryLogSink.h>

#include <array>
#include <chrono>
#include <cstdint>
#include <memory>
#include <optional>
#include <string>
#include <vector>

namespace Quasar::Editor
{
    struct EditorContext;

    enum class ConsoleCategory : std::uint8_t
    {
        Debug,
        Warning,
        Error
    };

    struct ConsoleCounts
    {
        std::size_t Debug = 0;
        std::size_t Warning = 0;
        std::size_t Error = 0;
    };

    class ConsolePanel final : public EditorPanel
    {
    public:
        explicit ConsolePanel(EditorContext& context);

    protected:
        void OnImGuiRender() override;

    private:
        struct MessageKey
        {
            std::chrono::system_clock::time_point Time;
            std::string LoggerName;
            std::string Text;
        };

        static MessageKey GetKey(const LogMessage& message);
        static bool IsSameMessage(const LogMessage& message, const MessageKey& key);
        static std::string FormatMessage(const LogMessage& message);

        std::vector<LogMessage> GetMessages() const;
        std::vector<LogMessage> FilterShown(std::vector<LogMessage> messages) const;
        void Clear();
        void ToggleExpanded(const LogMessage& message);

        void DrawToolbar(const ConsoleCounts& counts, const std::vector<LogMessage>& shown);
        void DrawFilterButton(ConsoleCategory category, std::size_t count);
        void DrawMessages(const std::vector<LogMessage>& messages);
        void DrawMessage(int index, const LogMessage& message);

        bool IsShown(ConsoleCategory category) const;
        bool IsExpanded(const LogMessage& message) const;

        std::shared_ptr<MemoryLogSink> m_Sink;
        std::array<bool, 3> m_Shown{true, true, true};
        std::optional<MessageKey> m_LastCleared;
        std::vector<MessageKey> m_Expanded;
    };
}
