#include <quasar/Version.h>
#include <quasar/core/Log.h>

#include <spdlog/sinks/basic_file_sink.h>
#include <spdlog/sinks/stdout_color_sinks.h>

#include <vector>

namespace Quasar
{
    std::shared_ptr<spdlog::logger> Log::s_EngineLogger;
    std::shared_ptr<spdlog::logger> Log::s_AppLogger;

    void Log::Init()
    {
        if (s_EngineLogger || s_AppLogger)
        {
            return;
        }

        auto console = std::make_shared<spdlog::sinks::stdout_color_sink_mt>();
        console->set_pattern("%^[%T] %n: %v%$");

        auto file = std::make_shared<spdlog::sinks::basic_file_sink_mt>("QuasarEngine.log", true);
        file->set_pattern("[%Y-%m-%d %T.%e] [%l] %n: %v");

        const std::vector<spdlog::sink_ptr> sinks{console, file};
        s_EngineLogger = std::make_shared<spdlog::logger>("ENGINE", sinks.begin(), sinks.end());
        s_AppLogger = std::make_shared<spdlog::logger>("APP", sinks.begin(), sinks.end());

        const auto level = PlatformDetection::IsDebug() ? spdlog::level::trace : spdlog::level::info;
        for (const auto& logger : {s_EngineLogger, s_AppLogger})
        {
            logger->set_level(level);
            logger->flush_on(spdlog::level::warn);
        }

        QUASAR_ENGINE_INFO("QuasarEngine v{}", GetVersionString());
    }

    void Log::Shutdown()
    {
        s_AppLogger.reset();
        s_EngineLogger.reset();
        spdlog::shutdown();
    }
}
