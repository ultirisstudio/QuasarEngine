#pragma once

/// \file
/// \brief The console panel of the editor, which shows the log messages of the engine and the application.
/// \author Fabien Wahbeh
/// \date 2026-11-10

#include <quasar/log/MemoryLogSink.h>

#include <chrono>
#include <memory>
#include <string>
#include <vector>

namespace Quasar::Editor
{
    /// @brief Panel that shows the most recent log messages kept by a MemoryLogSink.
    ///
    /// Each message takes one line: its time, its level, its logger and the first line of its text, cut at the edge
    /// of the panel. A click on a message shows its whole text, and a second click shows one line again. A right click
    /// on a message copies it.
    class ConsolePanel
    {
    public:
        /// @brief Creates a panel that shows the messages of a sink.
        /// @param sink The sink that keeps the log messages. With `nullptr`, the panel shows no message.
        explicit ConsolePanel(std::shared_ptr<MemoryLogSink> sink);

        /// @brief Draws the panel in an ImGui window named "Console".
        void OnImGuiRender();

        /// @brief Returns whether the whole text of a message is shown, instead of its first line only.
        bool IsExpanded(const LogMessage& message) const;

        /// @brief Returns the text copied for a message: its time, its level, its logger and its whole text.
        static std::string FormatMessage(const LogMessage& message);

    private:
        // A message is found again from one frame to the next by these fields, because the sink gives copies
        struct MessageKey
        {
            std::chrono::system_clock::time_point Time;
            std::string LoggerName;
            std::string Text;
        };

        static bool IsSameMessage(const LogMessage& message, const MessageKey& key);

        void ToggleExpanded(const LogMessage& message);
        void DrawMessages(const std::vector<LogMessage>& messages);
        void DrawMessage(int index, const LogMessage& message);

        std::shared_ptr<MemoryLogSink> m_Sink;
        std::vector<MessageKey> m_Expanded;
    };
}
