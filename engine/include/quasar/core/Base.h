#pragma once

#include <cstdint>
#include <memory>
#include <utility>

#include <quasar/core/Log.h>
#include <quasar/core/PlatformDetection.h>

#ifdef QUASAR_PLATFORM_LINUX
#include <csignal>
#endif

namespace Quasar
{
    template <typename T>
    using Scope = std::unique_ptr<T>;

    template <typename T, typename... Args>
    Scope<T> CreateScope(Args&&... args)
    {
        return std::make_unique<T>(std::forward<Args>(args)...);
    }

    template <typename T>
    using Ref = std::shared_ptr<T>;

    template <typename T, typename... Args>
    Ref<T> CreateRef(Args&&... args)
    {
        return std::make_shared<T>(std::forward<Args>(args)...);
    }

    // index must be lower than 32: shifting a 32-bit value by 32 or more is undefined behavior.
    constexpr std::uint32_t Bit(std::uint32_t index)
    {
        return 1u << index;
    }
}

#ifdef QUASAR_PLATFORM_WINDOWS
#define QUASAR_DEBUGBREAK() __debugbreak()
#define QUASAR_FORCE_INLINE __forceinline
#else
#define QUASAR_DEBUGBREAK() std::raise(SIGTRAP)
#define QUASAR_FORCE_INLINE [[gnu::always_inline]] inline
#endif

#ifdef QUASAR_DEBUG
#define QUASAR_ENGINE_ASSERT(condition, ...)                                                                           \
    do                                                                                                                 \
    {                                                                                                                  \
        if (!(condition))                                                                                              \
        {                                                                                                              \
            QUASAR_ENGINE_ERROR("Assertion '{}' failed at {}:{}", #condition, __FILE__, __LINE__);                     \
            if constexpr (sizeof(#__VA_ARGS__) > 1)                                                                    \
            {                                                                                                          \
                QUASAR_ENGINE_ERROR("" __VA_ARGS__);                                                                   \
            }                                                                                                          \
            QUASAR_DEBUGBREAK();                                                                                       \
        }                                                                                                              \
    } while (false)
#else
#define QUASAR_ENGINE_ASSERT(condition, ...) ((void)[&] { return !(condition); })
#endif
