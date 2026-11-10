#include <quasar/core/PlatformDetection.h>
#include <quasar/log/Log.h>

#include <doctest/doctest.h>

TEST_CASE("Init creates the engine and application loggers")
{
    Quasar::Log::Init();

    CHECK(&Quasar::Log::Engine() != &Quasar::Log::App());

    Quasar::Log::Shutdown();
}

TEST_CASE("The loggers write every level in Debug and from info in Release")
{
    Quasar::Log::Init();

    CHECK(Quasar::Log::Engine().level() == (Quasar::PlatformDetection::IsDebug() ? spdlog::level::trace : spdlog::level::info));

    Quasar::Log::Shutdown();
}

TEST_CASE("A value used only in a trace message compiles in every build type")
{
    Quasar::Log::Init();
    QUASAR_TRACE("value {}", 42);
    Quasar::Log::Shutdown();
}

TEST_CASE("Calling Init again keeps the same loggers")
{
    Quasar::Log::Init();
    const spdlog::logger* engine = &Quasar::Log::Engine();
    const spdlog::logger* app = &Quasar::Log::App();

    Quasar::Log::Init();

    CHECK(&Quasar::Log::Engine() == engine);
    CHECK(&Quasar::Log::App() == app);
    Quasar::Log::Shutdown();
}

TEST_CASE("Messages written before Init go to the fallback loggers")
{
    REQUIRE(Quasar::Log::GetMemorySink() == nullptr);

    CHECK(&Quasar::Log::Engine() != &Quasar::Log::App());
    QUASAR_ENGINE_INFO("Written before Init");
    QUASAR_ERROR("Written before Init");
}

TEST_CASE("Messages written after Shutdown go to the fallback loggers")
{
    Quasar::Log::Init();
    const spdlog::logger* engine = &Quasar::Log::Engine();
    Quasar::Log::Shutdown();

    CHECK(&Quasar::Log::Engine() != engine);
    QUASAR_ENGINE_WARN("Written after Shutdown");
}
