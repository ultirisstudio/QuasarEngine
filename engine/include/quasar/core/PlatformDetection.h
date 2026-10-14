#pragma once

#if defined(__linux__)
#define QUASAR_PLATFORM_LINUX
#elif defined(_WIN32)
#define QUASAR_PLATFORM_WINDOWS
#elif defined(__APPLE__)
#error "MacOS is not supported !"
#else
#error "Current platform is not supported !"
#endif

#ifdef NDEBUG
#define QUASAR_RELEASE
#else
#define QUASAR_DEBUG
#endif

namespace Quasar
{
    class PlatformDetection
    {
    public:
        static constexpr bool IsWindows()
        {
#ifdef QUASAR_PLATFORM_WINDOWS
            return true;
#else
            return false;
#endif
        }

        static constexpr bool IsLinux()
        {
#ifdef QUASAR_PLATFORM_LINUX
            return true;
#else
            return false;
#endif
        }

        static constexpr bool IsDebug()
        {
#ifdef QUASAR_DEBUG
            return true;
#else
            return false;
#endif
        }

        static constexpr bool IsRelease()
        {
#ifdef QUASAR_RELEASE
            return true;
#else
            return false;
#endif
        }
    };
}
