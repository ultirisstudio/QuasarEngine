#pragma once

/// \file
/// \brief Detects the platform and the build type at compile time.
/// \author Kurogenshi
/// \date 2026-10-19
///
/// This header defines one platform macro, QUASAR_PLATFORM_WINDOWS or QUASAR_PLATFORM_LINUX, and one build type
/// macro, QUASAR_DEBUG or QUASAR_RELEASE. The build type is Release when `NDEBUG` is defined. Other platforms,
/// including macOS, stop the build with an error.
///
/// Use these macros with `#ifdef` only for code that does not compile on every platform. Everywhere else, use the
/// PlatformDetection class.

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
    /// @brief Tells at compile time which platform and build type the engine is built for.
    /// The functions are constexpr, so they can be used with `if constexpr`. Unlike `#ifdef`, both branches are
    /// still compiled and checked on every platform.
    class PlatformDetection
    {
    public:
        /// @brief Returns true when the engine is built for Windows.
        /// @return
        static constexpr bool IsWindows()
        {
#ifdef QUASAR_PLATFORM_WINDOWS
            return true;
#else
            return false;
#endif
        }

        /// @brief Returns true when the engine is built for Linux.
        /// @return
        static constexpr bool IsLinux()
        {
#ifdef QUASAR_PLATFORM_LINUX
            return true;
#else
            return false;
#endif
        }

        /// @brief Returns true in a Debug build.
        /// @return
        static constexpr bool IsDebug()
        {
#ifdef QUASAR_DEBUG
            return true;
#else
            return false;
#endif
        }

        /// @brief Returns true in a Release build.
        /// @return
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
