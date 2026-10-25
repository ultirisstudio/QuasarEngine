#include <quasar/core/MemoryLogSink.h>

#include <algorithm>
#include <utility>

namespace Quasar
{
    namespace
    {
        LogLevel ToLogLevel(spdlog::level::level_enum level)
        {
            switch (level)
            {
                case spdlog::level::trace:
                    return LogLevel::Trace;
                case spdlog::level::debug:
                    return LogLevel::Debug;
                case spdlog::level::warn:
                    return LogLevel::Warning;
                case spdlog::level::err:
                    return LogLevel::Error;
                case spdlog::level::critical:
                    return LogLevel::Critical;
                default:
                    return LogLevel::Info;
            }
        }
    }

    MemoryLogSink::MemoryLogSink(std::size_t capacity) : m_Capacity(std::max<std::size_t>(capacity, 1)) {}

    std::vector<LogMessage> MemoryLogSink::GetMessages() const
    {
        const std::lock_guard lock(m_MessagesMutex);
        return {m_Messages.begin(), m_Messages.end()};
    }

    void MemoryLogSink::Clear()
    {
        const std::lock_guard lock(m_MessagesMutex);
        m_Messages.clear();
    }

    void MemoryLogSink::sink_it_(const spdlog::details::log_msg& message)
    {
        LogMessage kept;
        kept.Level = ToLogLevel(message.level);
        kept.Time = message.time;
        kept.LoggerName.assign(message.logger_name.data(), message.logger_name.size());
        kept.Text.assign(message.payload.data(), message.payload.size());

        const std::lock_guard lock(m_MessagesMutex);
        if (m_Messages.size() == m_Capacity)
        {
            m_Messages.pop_front();
        }
        m_Messages.push_back(std::move(kept));
    }
}