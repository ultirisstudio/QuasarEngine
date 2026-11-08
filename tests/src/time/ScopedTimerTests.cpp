#include <quasar/core/PlatformDetection.h>
#include <quasar/log/Log.h>
#include <quasar/time/ScopedTimer.h>

#include <doctest/doctest.h>

#include <spdlog/sinks/ostream_sink.h>

#include <chrono>
#include <cstddef>
#include <memory>
#include <optional>
#include <sstream>
#include <string>
#include <thread>
#include <type_traits>

static_assert(!std::is_copy_constructible_v<Quasar::ScopedTimer>, "A ScopedTimer must measure a single scope");
static_assert(!std::is_move_constructible_v<Quasar::ScopedTimer>, "A ScopedTimer must measure a single scope");

namespace
{
    class LogCapture
    {
    public:
        LogCapture()
        {
            Quasar::Log::Init();
            auto sink = std::make_shared<spdlog::sinks::ostream_sink_mt>(m_Stream);
            sink->set_pattern("%v");
            Quasar::Log::Engine().sinks().push_back(sink);
        }

        ~LogCapture() { Quasar::Log::Shutdown(); }

        LogCapture(const LogCapture&) = delete;
        LogCapture& operator=(const LogCapture&) = delete;
        LogCapture(LogCapture&&) = delete;
        LogCapture& operator=(LogCapture&&) = delete;

        std::string GetText() const { return m_Stream.str(); }

    private:
        std::ostringstream m_Stream;
    };

    std::optional<double> ReadDuration(const std::string& text, const std::string& name)
    {
        const std::size_t namePosition = text.find(name);
        if (namePosition == std::string::npos)
        {
            return std::nullopt;
        }

        const std::size_t numberPosition = text.find_first_of("0123456789", namePosition + name.size());
        if (numberPosition == std::string::npos)
        {
            return std::nullopt;
        }

        return std::stod(text.substr(numberPosition));
    }

    bool Contains(const std::string& text, const std::string& part)
    {
        return text.find(part) != std::string::npos;
    }
}

TEST_CASE("A ScopedTimer logs nothing before its scope ends")
{
    const LogCapture capture;

    const Quasar::ScopedTimer timer("Loading");

    CHECK(capture.GetText().empty());
}

TEST_CASE("A ScopedTimer logs the name chosen by the caller when its scope ends")
{
    const LogCapture capture;

    {
        const Quasar::ScopedTimer timer("Loading");
    }

    CHECK(ReadDuration(capture.GetText(), "Loading").has_value());
}

TEST_CASE("A ScopedTimer measures at least the time spent in its scope")
{
    const LogCapture capture;

    {
        const Quasar::ScopedTimer timer("Sleep");
        std::this_thread::sleep_for(std::chrono::milliseconds(20));
    }

    const std::optional<double> milliseconds = ReadDuration(capture.GetText(), "Sleep");
    REQUIRE(milliseconds.has_value());
    CHECK(*milliseconds >= 20.0);
}

TEST_CASE("QUASAR_SCOPED_TIMER logs the name when its scope ends only in Debug")
{
    const LogCapture capture;

    {
        QUASAR_SCOPED_TIMER("Loading");
    }

    if (Quasar::PlatformDetection::IsDebug())
    {
        CHECK(Contains(capture.GetText(), "Loading"));
    }
    else
    {
        CHECK(capture.GetText().empty());
    }
}

TEST_CASE("Several QUASAR_SCOPED_TIMER can be used in the same scope")
{
    const LogCapture capture;

    {
        QUASAR_SCOPED_TIMER("First");
        QUASAR_SCOPED_TIMER("Second");
    }

    const std::string text = capture.GetText();
    CHECK(Contains(text, "First") == Quasar::PlatformDetection::IsDebug());
    CHECK(Contains(text, "Second") == Quasar::PlatformDetection::IsDebug());
}

TEST_CASE("A name used only by QUASAR_SCOPED_TIMER compiles in every build type")
{
    const LogCapture capture;
    const char* const name = "Loading";

    {
        QUASAR_SCOPED_TIMER(name);
    }

    CHECK(capture.GetText().empty() == Quasar::PlatformDetection::IsRelease());
}
