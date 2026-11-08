#pragma once

#include <quasar/log/Log.h>
#include <quasar/log/MemoryLogSink.h>

#include <algorithm>
#include <memory>
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
}
