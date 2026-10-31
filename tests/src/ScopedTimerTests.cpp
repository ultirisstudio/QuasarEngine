#include <quasar/core/Log.h>
#include <quasar/core/PlatformDetection.h>
#include <quasar/core/ScopedTimer.h>

#include <doctest/doctest.h>

#include <spdlog/sinks/ostream_sink.h>

#include <chrono>
#include <memory>
#include <sstream>
#include <string>
#include <thread>
#include <type_traits>

static_assert(!std::is_copy_constructible_v<Quasar::ScopedTimer>, "A ScopedTimer must measure a single scope");
static_assert(!std::is_move_constructible_v<Quasar::ScopedTimer>, "A ScopedTimer must measure a single scope");

namespace
{
    // Captures the engine log messages in a string, with only the message text
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

    CHECK(capture.GetText().starts_with("Loading took "));
    CHECK(capture.GetText().find(" ms") != std::string::npos);
}

TEST_CASE("A ScopedTimer measures at least the time spent in its scope")
{
    const LogCapture capture;

    {
        const Quasar::ScopedTimer timer("Sleep");
        std::this_thread::sleep_for(std::chrono::milliseconds(20));
    }

    const std::string text = capture.GetText();
    const std::string prefix = "Sleep took ";
    REQUIRE(text.starts_with(prefix));
    CHECK(std::stod(text.substr(prefix.size())) >= 20.0);
}

TEST_CASE("QUASAR_SCOPED_TIMER logs the name when its scope ends only in Debug")
{
    const LogCapture capture;

    {
        QUASAR_SCOPED_TIMER("Loading");
    }

    if (Quasar::PlatformDetection::IsDebug())
    {
        CHECK(capture.GetText().starts_with("Loading took "));
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
    const bool firstLogged = text.find("First took ") != std::string::npos;
    const bool secondLogged = text.find("Second took ") != std::string::npos;
    CHECK(firstLogged == Quasar::PlatformDetection::IsDebug());
    CHECK(secondLogged == Quasar::PlatformDetection::IsDebug());
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
