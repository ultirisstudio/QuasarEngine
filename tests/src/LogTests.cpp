#include <quasar/core/PlatformDetection.h>
#include <quasar/core/Log.h>

#include <doctest/doctest.h>

TEST_CASE("Loggers are initialized correctly")
{
    Quasar::Log::Init();

    CHECK(Quasar::Log::Engine().name() == "ENGINE");
    CHECK(Quasar::Log::App().name() == "APP");

    Quasar::Log::Shutdown();
}

TEST_CASE("Loggers have the correct log level based on build configuration")
{
    Quasar::Log::Init();

    CHECK(Quasar::Log::Engine().level() == (Quasar::PlatformDetection::IsDebug() ? spdlog::level::trace : spdlog::level::info));

    Quasar::Log::Shutdown();
}
