#include <quasar/core/Log.h>
#include <quasar/core/MemoryLogSink.h>

#include <doctest/doctest.h>
#include <spdlog/spdlog.h>

#include <chrono>
#include <cstddef>
#include <memory>
#include <string>
#include <thread>
#include <vector>

using Quasar::LogLevel;
using Quasar::LogMessage;
using Quasar::MemoryLogSink;

namespace
{
    spdlog::logger MakeLogger(const std::shared_ptr<MemoryLogSink>& sink)
    {
        spdlog::logger logger("TEST", sink);
        logger.set_level(spdlog::level::trace);
        return logger;
    }
}

TEST_CASE("The memory sink keeps messages with their level and time")
{
    const auto sink = std::make_shared<MemoryLogSink>(10);
    spdlog::logger logger = MakeLogger(sink);

    const auto before = std::chrono::system_clock::now();
    logger.info("Loaded {} meshes", 3);
    logger.warn("Missing texture");
    const auto after = std::chrono::system_clock::now();

    const std::vector<LogMessage> messages = sink->GetMessages();
    REQUIRE(messages.size() == 2);
    CHECK(messages[0].Level == LogLevel::Info);
    CHECK(messages[0].Text == "Loaded 3 meshes");
    CHECK(messages[0].LoggerName == "TEST");
    CHECK(messages[1].Level == LogLevel::Warning);
    CHECK(messages[1].Text == "Missing texture");
    for (const LogMessage& message : messages)
    {
        CHECK(message.Time >= before);
        CHECK(message.Time <= after);
    }
}

TEST_CASE("The memory sink converts every level")
{
    const auto sink = std::make_shared<MemoryLogSink>(10);
    spdlog::logger logger = MakeLogger(sink);

    logger.trace("trace");
    logger.debug("debug");
    logger.info("info");
    logger.warn("warning");
    logger.error("error");
    logger.critical("critical");

    const std::vector<LogMessage> messages = sink->GetMessages();
    REQUIRE(messages.size() == 6);
    CHECK(messages[0].Level == LogLevel::Trace);
    CHECK(messages[1].Level == LogLevel::Debug);
    CHECK(messages[2].Level == LogLevel::Info);
    CHECK(messages[3].Level == LogLevel::Warning);
    CHECK(messages[4].Level == LogLevel::Error);
    CHECK(messages[5].Level == LogLevel::Critical);
}

TEST_CASE("The oldest messages are dropped when the limit is reached")
{
    const auto sink = std::make_shared<MemoryLogSink>(3);
    spdlog::logger logger = MakeLogger(sink);

    for (int i = 1; i <= 5; ++i)
    {
        logger.info("Message {}", i);
    }

    const std::vector<LogMessage> messages = sink->GetMessages();
    REQUIRE(messages.size() == 3);
    CHECK(messages[0].Text == "Message 3");
    CHECK(messages[1].Text == "Message 4");
    CHECK(messages[2].Text == "Message 5");
}

TEST_CASE("The memory sink can be cleared")
{
    const auto sink = std::make_shared<MemoryLogSink>(3);
    spdlog::logger logger = MakeLogger(sink);
    logger.info("Message");

    sink->Clear();

    CHECK(sink->GetMessages().empty());
    CHECK(sink->GetCapacity() == 3);
}

TEST_CASE("Messages can be read while new ones are being logged")
{
    constexpr std::size_t capacity = 100;
    constexpr int messageCount = 20'000;
    const auto sink = std::make_shared<MemoryLogSink>(capacity);
    spdlog::logger logger = MakeLogger(sink);

    std::jthread writer(
        [&]
        {
            for (int i = 0; i < messageCount; ++i)
            {
                logger.info("{}", i);
            }
        });

    bool consistent = true;
    for (int read = 0; read < 1'000; ++read)
    {
        const std::vector<LogMessage> messages = sink->GetMessages();
        consistent = consistent && messages.size() <= capacity;
        for (std::size_t i = 1; i < messages.size(); ++i)
        {
            consistent = consistent && std::stoi(messages[i].Text) == std::stoi(messages[i - 1].Text) + 1;
        }
    }
    writer.join();

    CHECK(consistent);
    const std::vector<LogMessage> messages = sink->GetMessages();
    REQUIRE(messages.size() == capacity);
    CHECK(messages.back().Text == std::to_string(messageCount - 1));
}

TEST_CASE("The engine loggers write to the memory sink")
{
    Quasar::Log::Init();
    const std::shared_ptr<MemoryLogSink> sink = Quasar::Log::GetMemorySink();
    REQUIRE(sink != nullptr);
    sink->Clear();

    QUASAR_ENGINE_WARN("Engine warning");
    QUASAR_ERROR("Application error");

    const std::vector<LogMessage> messages = sink->GetMessages();
    REQUIRE(messages.size() == 2);
    CHECK(messages[0].LoggerName == "ENGINE");
    CHECK(messages[0].Text == "Engine warning");
    CHECK(messages[1].LoggerName == "APP");
    CHECK(messages[1].Level == LogLevel::Error);
    CHECK(sink->GetCapacity() == Quasar::Log::MemoryCapacity);

    Quasar::Log::Shutdown();
    CHECK(Quasar::Log::GetMemorySink() == nullptr);
}
