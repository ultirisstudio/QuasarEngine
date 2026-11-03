#pragma once

#include <quasar/core/Log.h>
#include <quasar/core/MemoryLogSink.h>

#include <algorithm>
#include <memory>
#include <vector>

namespace TestHelpers
{
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