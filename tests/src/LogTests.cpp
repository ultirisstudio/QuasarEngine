#include <quasar/core/Log.h>
#include <quasar/core/PlatformDetection.h>

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

    CHECK(Quasar::Log::Engine().level() ==
          (Quasar::PlatformDetection::IsDebug() ? spdlog::level::trace : spdlog::level::info));

    Quasar::Log::Shutdown();
}

TEST_CASE("A value used only in a trace message compiles in every build type")
{
    Quasar::Log::Init();
    QUASAR_TRACE("value {}", 42);
    Quasar::Log::Shutdown();
}
