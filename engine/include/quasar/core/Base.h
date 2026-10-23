#pragma once

/// \file
/// \brief Contains the base definitions and utilities for the Quasar engine.
/// \author Kurogenshi
/// \date 2026-10-19

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
    /// @brief Pointer with a single owner. Alias of `std::unique_ptr`.
    /// @tparam T
    template <typename T>
    using Scope = std::unique_ptr<T>;

    /// @brief Creates an object owned by a Scope. Same as `std::make_unique`.
    /// @tparam T
    /// @tparam ...Args
    /// @param ...args
    /// @return
    template <typename T, typename... Args>
    Scope<T> CreateScope(Args&&... args)
    {
        return std::make_unique<T>(std::forward<Args>(args)...);
    }

    /// @brief Pointer with shared ownership: the object is destroyed with its last Ref. Alias of `std::shared_ptr`.
    /// @tparam T
    template <typename T>
    using Ref = std::shared_ptr<T>;

    /// @brief Creates an object owned by a Ref. Same as `std::make_shared`.
    /// @tparam T
    /// @tparam ...Args
    /// @param ...args
    /// @return
    template <typename T, typename... Args>
    Ref<T> CreateRef(Args&&... args)
    {
        return std::make_shared<T>(std::forward<Args>(args)...);
    }

    /// @brief Returns a value where only the bit at the given index is set, for example `Bit(3)` is 8.
    ///
    /// It is used to build flags that are combined with the `|` operator. The index must be lower than 32:
    /// shifting a 32-bit value by 32 or more is undefined behavior.
    /// @param index
    /// @return
    constexpr std::uint32_t Bit(std::uint32_t index)
    {
        return 1u << index;
    }
}

/// \def QUASAR_DEBUGBREAK()
/// @brief Stops the program at this line in the debugger. Without a debugger, the program is terminated.

/// \def QUASAR_FORCE_INLINE
/// @brief Asks the compiler to always inline a function.
#ifdef QUASAR_PLATFORM_WINDOWS
#define QUASAR_DEBUGBREAK() __debugbreak()
#define QUASAR_FORCE_INLINE __forceinline
#else
#define QUASAR_DEBUGBREAK() std::raise(SIGTRAP)
#define QUASAR_FORCE_INLINE [[gnu::always_inline]] inline
#endif

/// @brief Joins two tokens with `##`. Use QUASAR_CONCAT() instead, which expands the macros first.
#define QUASAR_CONCAT_IMPL(a, b) a##b

/// @brief Joins two tokens into one, after expanding the macros they contain.
///
/// `QUASAR_CONCAT(timer, __LINE__)` gives `timer42` on line 42. Writing `timer##__LINE__` directly would give
/// `timer__LINE__`, because `##` joins the tokens before `__LINE__` is replaced by its value.
#define QUASAR_CONCAT(a, b) QUASAR_CONCAT_IMPL(a, b)

/// \def QUASAR_ENGINE_ASSERT(condition, ...)
/// @brief Checks a condition that must always be true in the engine code.
///
/// In Debug, when the condition is false, the condition, the file and the line are logged as an error, followed
/// by the optional message, then the program stops with QUASAR_DEBUGBREAK(). The message uses the fmt syntax:
/// `QUASAR_ENGINE_ASSERT(index < count, "Index {} is out of range", index)`.
///
/// In Release, the condition is not evaluated, so it must not have side effects.
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
