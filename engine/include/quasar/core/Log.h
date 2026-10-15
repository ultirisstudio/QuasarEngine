#pragma once

#include <quasar/core/PlatformDetection.h>

#include <spdlog/spdlog.h>

#include <memory>

namespace Quasar
{
    class Log
    {
    public:
        static void Init();
        static void Shutdown();

        static spdlog::logger& Engine() { return *s_EngineLogger; }
        static spdlog::logger& App() { return *s_AppLogger; }

    private:
        static std::shared_ptr<spdlog::logger> s_EngineLogger;
        static std::shared_ptr<spdlog::logger> s_AppLogger;
    };
}

#ifdef QUASAR_DEBUG
#define QUASAR_ENGINE_TRACE(...) ::Quasar::Log::Engine().trace(__VA_ARGS__)
#define QUASAR_TRACE(...) ::Quasar::Log::App().trace(__VA_ARGS__)
#else
#define QUASAR_ENGINE_TRACE(...) ((void)0)
#define QUASAR_TRACE(...) ((void)0)
#endif

#define QUASAR_ENGINE_INFO(...) ::Quasar::Log::Engine().info(__VA_ARGS__)
#define QUASAR_ENGINE_WARN(...) ::Quasar::Log::Engine().warn(__VA_ARGS__)
#define QUASAR_ENGINE_ERROR(...) ::Quasar::Log::Engine().error(__VA_ARGS__)

#define QUASAR_INFO(...) ::Quasar::Log::App().info(__VA_ARGS__)
#define QUASAR_WARN(...) ::Quasar::Log::App().warn(__VA_ARGS__)
#define QUASAR_ERROR(...) ::Quasar::Log::App().error(__VA_ARGS__)
