#include <quasar/core/PlatformDetection.h>

#include <doctest/doctest.h>

TEST_CASE("Exactly one platform is detected")
{
    CHECK(Quasar::PlatformDetection::IsWindows() != Quasar::PlatformDetection::IsLinux());
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
   