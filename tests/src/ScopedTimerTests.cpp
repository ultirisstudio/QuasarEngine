#include <quasar/core/Log.h>
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
