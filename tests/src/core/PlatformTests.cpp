#include <quasar/core/PlatformDetection.h>

#include <doctest/doctest.h>

TEST_CASE("Exactly one platform is detected")
{
    const int platforms = static_cast<int>(Quasar::PlatformDetection::IsWindows()) + static_cast<int>(Quasar::PlatformDetection::IsLinux()) +
                          static_cast<int>(Quasar::PlatformDetection::IsAndroid());

    CHECK(platforms == 1);
}

TEST_CASE("The build configuration matches NDEBUG")
{
#ifdef NDEBUG
    CHECK(Quasar::PlatformDetection::IsRelease());
#else
    CHECK(Quasar::PlatformDetection::IsDebug());
#endif
    CHECK(Quasar::PlatformDetection::IsDebug() != Quasar::PlatformDetection::IsRelease());
}
