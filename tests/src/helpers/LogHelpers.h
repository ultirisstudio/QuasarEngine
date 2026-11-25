#pragma once

#include <quasar/log/Log.h>
#include <quasar/log/MemoryLogSink.h>

#include <algorithm>
#include <cstddef>
#include <memory>
#include <string_view>
#include <vector>

namespace TestHelpers
{
    class ScopedLog
    {
    public:
        ScopedLog() { Quasar::Log::Init(); }
        ~ScopedLog() { Quasar::Log::Shutdown(); }

        ScopedLog(const ScopedLog&) = delete;
        ScopedLog& operator=(const ScopedLog&) = delete;
        ScopedLog(ScopedLog&&) = delete;
        ScopedLog& operator=(ScopedLog&&) = delete;
    };

    inline bool HasLogAtLeast(Quasar::LogLevel level)
    {
        const std::shared_ptr<Quasar::MemoryLogSink> sink = Quasar::Log::GetMemorySink();
        if (sink == nullptr)
        {
            return false;
        }

        const std::vector<Quasar::LogMessage> messages = sink->GetMessages();
        return std::ranges::any_of(messages, [level](const Quasar::LogMessage& message) { return message.Level >= level; });
    }

    inline std::ptrdiff_t CountLogs(Quasar::LogLevel level, std::string_view text = {})
    {
        const std::shared_ptr<Quasar::MemoryLogSink> sink = Quasar::Log::GetMemorySink();
        if (sink == nullptr)
        {
            return 0;
        }

        const std::vector<Quasar::LogMessage> messages = sink->GetMessages();
        return std::ranges::count_if(messages, [level, text](const Quasar::LogMessage& message)
                                     { return message.Level == level && message.Text.find(text) != std::string_view::npos; });
    }

    inline std::ptrdiff_t CountErrors(std::string_view text = {})
    {
        return CountLogs(Quasar::LogLevel::Error, text);
    }

    inline std::ptrdiff_t CountWarnings(std::string_view text = {})
    {
        return CountLogs(Quasar::LogLevel::Warning, text);
    }
}
