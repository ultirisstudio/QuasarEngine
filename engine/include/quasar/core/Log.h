#pragma once

/// \file
/// \brief Engine and application loggers, and the macros to write to them.
/// \author Kurogenshi
/// \date 2026-10-19
///
/// The macros take a format string and its arguments, with the fmt syntax:
/// `QUASAR_ENGINE_INFO("Loaded {} meshes", count)`. Engine code uses the `QUASAR_ENGINE_` macros, and
/// application code uses the `QUASAR_` macros. Log::Init() must be called before the first message.

#include <quasar/core/PlatformDetection.h>

#include <spdlog/spdlog.h>

#include <memory>

namespace Quasar
{
    /// @brief The loggers for the engine and the application.
    /// Owns the two loggers: one for engine messages and one for application messages.
    ///
    /// Messages are written to the console and to the file `QuasarEngine.log`. In Debug, every level is written.
    /// In Release, only info, warning and error messages are written.
    class Log
    {
    public:
        /// @brief Creates the loggers. Calling it again does nothing.
        static void Init();

        /// @brief Destroys the loggers. No message can be written after this call, until Init() is called again.
        static void Shutdown();

        /// @brief Returns the logger for engine messages. Init() must have been called.
        /// @return
        static spdlog::logger& Engine() { return *s_EngineLogger; }

        /// @brief Returns the logger for application messages. Init() must have been called.
        /// @return
        static spdlog::logger& App() { return *s_AppLogger; }

    private:
        static std::shared_ptr<spdlog::logger> s_EngineLogger;
        static std::shared_ptr<spdlog::logger> s_AppLogger;
    };
}

/// \def QUASAR_ENGINE_TRACE(...)
/// @brief Writes a trace message from the engine. Removed from Release builds, where its arguments are still checked
/// by the compiler but never evaluated.

/// \def QUASAR_TRACE(...)
/// @brief Writes a trace message from the application. Removed from Release builds, where its arguments are still
/// checked by the compiler but never evaluated.
#ifdef QUASAR_DEBUG
#define QUASAR_ENGINE_TRACE(...) ::Quasar::Log::Engine().trace(__VA_ARGS__)
#define QUASAR_TRACE(...) ::Quasar::Log::App().trace(__VA_ARGS__)
#else
#define QUASAR_ENGINE_TRACE(...)
do
{
    if constexpr (false)
    {
        ::Quasar::Log::Engine().trace(__VA_ARGS__);
    }
} while (false)
#define QUASAR_TRACE(...)
    do
{
    if constexpr (false)
    {
        ::Quasar::Log::App().trace(__VA_ARGS__);
    }
}
while (false)
#endif

/// \def QUASAR_ENGINE_INFO(...)
/// @brief Writes an info message from the engine. The message is written in Debug and Release builds.

/// \def QUASAR_ENGINE_WARN(...)
/// @brief Writes a warning message from the engine. The message is written in Debug and Release builds.

/// \def QUASAR_ENGINE_ERROR(...)
/// @brief Writes an error message from the engine. The message is written in Debug and Release builds.

#define QUASAR_ENGINE_INFO(...) ::Quasar::Log::Engine().info(__VA_ARGS__)
#define QUASAR_ENGINE_WARN(...) ::Quasar::Log::Engine().warn(__VA_ARGS__)
#define QUASAR_ENGINE_ERROR(...) ::Quasar::Log::Engine().error(__VA_ARGS__)

/// \def QUASAR_INFO(...)
/// @brief Writes an info message from the application. The message is written in Debug and Release builds.

/// \def QUASAR_WARN(...)
/// @brief Writes a warning message from the application. The message is written in Debug and Release builds.

/// \def QUASAR_ERROR(...)
/// @brief Writes an error message from the application. The message is written in Debug and Release builds.

#define QUASAR_INFO(...) ::Quasar::Log::App().info(__VA_ARGS__)
#define QUASAR_WARN(...) ::Quasar::Log::App().warn(__VA_ARGS__)
#define QUASAR_ERROR(...) ::Quasar::Log::App().error(__VA_ARGS__)
