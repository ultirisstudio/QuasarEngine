#pragma once

/// \file
/// \brief Log sink that keeps the most recent messages in memory, for the console.
/// \author Kurogenshi
/// \date 2026-10-25

#include <spdlog/sinks/base_sink.h>

#include <chrono>
#include <cstddef>
#include <cstdint>
#include <deque>
#include <mutex>
#include <string>
#include <vector>

namespace Quasar
{
    /// @brief Severity of a log message.
    enum class LogLevel : std::uint8_t
    {
        Trace,   ///< Detailed information, only written in Debug builds.
        Debug,   ///< Information useful while debugging.
        Info,    ///< Normal information.
        Warning, ///< Something unexpected that does not stop the program.
        Error,   ///< Something failed.
        Critical ///< Something failed and the program cannot continue.
    };

    /// @brief A log message kept by MemoryLogSink.
    struct LogMessage
    {
        LogLevel Level = LogLevel::Info;            ///< Severity of the message.
        std::chrono::system_clock::time_point Time; ///< Time when the message was written.
        std::string LoggerName;                     ///< Name of the logger, "ENGINE" or "APP".
        std::string Text;                           ///< Text of the message, without the time or the level.
    };

    /// @brief Log sink that keeps the most recent messages in memory, so that the console can display them.
    ///
    /// The sink keeps at most a given number of messages: when it is full, each new message replaces the oldest one.
    /// Messages can be read from any thread, even while other threads are writing new ones.
    class MemoryLogSink final : public spdlog::sinks::base_sink<std::mutex>
    {
    public:
        /// @brief Creates a sink that keeps at most `capacity` messages. A capacity of 0 keeps one message.
        explicit MemoryLogSink(std::size_t capacity);

        /// @brief Returns a copy of the kept messages, from the oldest to the most recent.
        std::vector<LogMessage> GetMessages() const;

        /// @brief Removes every kept message.
        void Clear();

        /// @brief Returns the maximum number of messages the sink keeps.
        std::size_t GetCapacity() const { return m_Capacity; }

    protected:
        /// @brief Keeps a new message. Called by spdlog for each message written to a logger that uses this sink.
        void sink_it_(const spdlog::details::log_msg& message) override;

        /// @brief Does nothing: the messages are already in memory.
        void flush_() override {}

    private:
        const std::size_t m_Capacity;
        mutable std::mutex m_MessagesMutex;
        std::deque<LogMessage> m_Messages;
    };
}
