#include <quasar/Version.h>
#include <quasar/core/Log.h>
#include <quasar/core/MemoryLogSink.h>

#include <spdlog/sinks/basic_file_sink.h>
#include <spdlog/sinks/stdout_color_sinks.h>

#include <memory>
#include <string>
#include <utility>
#include <vector>

namespace Quasar
{
    namespace
    {
        spdlog::logger CreateFallbackLogger(std::string name)
        {
            spdlog::logger logger(std::move(name), std::make_shared<spdlog::sinks::stderr_color_sink_mt>());
            logger.set_pattern("%^[%T] %n: %v%$");
            return logger;
        }
    }

    std::shared_ptr<spdlog::logger> Log::s_EngineLogger;
    std::shared_ptr<spdlog::logger> Log::s_AppLogger;
    std::shared_ptr<MemoryLogSink> Log::s_MemorySink;

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

        s_MemorySink = std::make_shared<MemoryLogSink>(MemoryCapacity);

        const std::vector<spdlog::sink_ptr> sinks{console, file, s_MemorySink};
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

    spdlog::logger& Log::GetFallbackEngineLogger()
    {
        static spdlog::logger logger = CreateFallbackLogger("ENGINE");
        return logger;
    }

    spdlog::logger& Log::GetFallbackAppLogger()
    {
        static spdlog::logger logger = CreateFallbackLogger("APP");
        return logger;
    }

    void Log::Shutdown()
    {
        s_AppLogger.reset();
        s_EngineLogger.reset();
        s_MemorySink.reset();
        spdlog::shutdown();
    }
}
