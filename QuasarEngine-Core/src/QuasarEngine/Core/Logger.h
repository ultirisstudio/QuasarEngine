#pragma once

#include <string>
#include <string_view>
#include <vector>
#include <ostream>
#include <mutex>
#include <atomic>
#include <cstdint>

#ifndef Q_LOG_COMPILE_LEVEL
#define Q_LOG_COMPILE_LEVEL 0
#endif

#ifndef Q_LOG_SHORT_FILE
#define Q_LOG_SHORT_FILE 1
#endif

#ifndef Q_LOG_WITH_THREAD_ID
#define Q_LOG_WITH_THREAD_ID 1
#endif

namespace QuasarEngine {

    class Logger {
    public:
        enum class Level : int32_t {
            Debug = 0,
            Info = 1,
            Warning = 2,
            Error = 3,
            Fatal = 4
        };

        static void setMinLevel(Level level) noexcept;
        static Level minLevel() noexcept;

        static void enableColors(bool enabled) noexcept;
        static bool colorsEnabled() noexcept;

        static void setAbortOnFatal(bool enabled) noexcept;
        static bool abortOnFatal() noexcept;

        static void addSink(std::ostream& os);
        static bool addFileSink(const std::string& path, bool truncate = false);
        static void clearSinks();

        static void initUtf8Console() noexcept;

        static void log(Level level, std::string_view message, const char* file, int line) noexcept;

#if defined(__cpp_lib_format) && (__cpp_lib_format >= 201907L)
        template <class... Args>
        static void logf(Level level, const char* file, int line, std::string_view fmt, Args&&... args) noexcept {
            if (!shouldLog(level)) return;
            try {
                auto msg = std::vformat(fmt, std::make_format_args(std::forward<Args>(args)...));
                log(level, msg, file, line);
            }
            catch (...) {
                log(level, fmt, file, line);
            }
        }
#endif

        static void checkAssert(bool condition, std::string_view message, const char* file, int line) noexcept;

        static void apiInfo(std::string_view apiName, std::initializer_list<std::pair<std::string_view, std::string_view>> fields) noexcept;

        static constexpr const char* levelToString(Level lvl) noexcept {
            switch (lvl) {
            case Level::Debug:   return "DEBUG";
            case Level::Info:    return "INFO";
            case Level::Warning: return "WARNING";
            case Level::Error:   return "ERROR";
            case Level::Fatal:   return "FATAL";
            default:             return "UNKNOWN";
            }
        }

        static bool shouldLog(Level level) noexcept;

    private:
        Logger() = delete;

        struct Sink {
            std::ostream* stream = nullptr;
            bool owned = false;
        };

        static void writeLine(Level level, std::string_view header, std::string_view message) noexcept;
        static std::string makeHeader(Level level, const char* file, int line) noexcept;

        static const char* levelColor(Level level) noexcept;

        static std::tm localtimeSafe(std::time_t t) noexcept;

        static std::mutex& mutex_();
        static std::vector<Sink>& sinks_();
        static std::atomic<int32_t>& minLevel_();
        static std::atomic<bool>& colors_();
        static std::atomic<bool>& abortOnFatal_();
        static bool isTerminal() noexcept;
        static void tryEnableVTOnWindows() noexcept;
    };
}

#define Q_LOG(level, msg) ::QuasarEngine::Logger::log(level, (msg), __FILE__, __LINE__)

#if (Q_LOG_COMPILE_LEVEL <= 0)
#define Q_DEBUG(msg) ::QuasarEngine::Logger::log(::QuasarEngine::Logger::Level::Debug, (msg), __FILE__, __LINE__)
#else
#define Q_DEBUG(msg) ((void)0)
#endif

#if (Q_LOG_COMPILE_LEVEL <= 1)
#define Q_INFO(msg)  ::QuasarEngine::Logger::log(::QuasarEngine::Logger::Level::Info,  (msg), __FILE__, __LINE__)
#else
#define Q_INFO(msg)  ((void)0)
#endif

#if (Q_LOG_COMPILE_LEVEL <= 2)
#define Q_WARNING(msg) ::QuasarEngine::Logger::log(::QuasarEngine::Logger::Level::Warning, (msg), __FILE__, __LINE__)
#else
#define Q_WARNING(msg) ((void)0)
#endif

#if (Q_LOG_COMPILE_LEVEL <= 3)
#define Q_ERROR(msg) ::QuasarEngine::Logger::log(::QuasarEngine::Logger::Level::Error, (msg), __FILE__, __LINE__)
#else
#define Q_ERROR(msg) ((void)0)
#endif

#if (Q_LOG_COMPILE_LEVEL <= 4)
#define Q_FATAL(msg) ::QuasarEngine::Logger::log(::QuasarEngine::Logger::Level::Fatal, (msg), __FILE__, __LINE__)
#else
#define Q_FATAL(msg) ((void)0)
#endif

#ifndef NDEBUG
#define Q_ASSERT(cond, msg) ::QuasarEngine::Logger::checkAssert((cond), (msg), __FILE__, __LINE__)
#else
#define Q_ASSERT(cond, msg) ((void)0)
#endif

#if defined(__cpp_lib_format) && (__cpp_lib_format >= 201907L)
#include <format>
#define Q_DEBUGF(fmt, ...) ::QuasarEngine::Logger::logf(::QuasarEngine::Logger::Level::Debug,  __FILE__, __LINE__, (fmt), __VA_ARGS__)
#define Q_INFOF(fmt, ...)  ::QuasarEngine::Logger::logf(::QuasarEngine::Logger::Level::Info,   __FILE__, __LINE__, (fmt), __VA_ARGS__)
#define Q_WARNF(fmt, ...)  ::QuasarEngine::Logger::logf(::QuasarEngine::Logger::Level::Warning,__FILE__, __LINE__, (fmt), __VA_ARGS__)
#define Q_ERRF(fmt, ...)   ::QuasarEngine::Logger::logf(::QuasarEngine::Logger::Level::Error,  __FILE__, __LINE__, (fmt), __VA_ARGS__)
#define Q_FATALF(fmt, ...) ::QuasarEngine::Logger::logf(::QuasarEngine::Logger::Level::Fatal,  __FILE__, __LINE__, (fmt), __VA_ARGS__)
#else
#define Q_DEBUGF(fmt, ...) ((void)0)
#define Q_INFOF(fmt, ...)  ((void)0)
#define Q_WARNF(fmt, ...)  ((void)0)
#define Q_ERRF(fmt, ...)   ((void)0)
#define Q_FATALF(fmt, ...) ((void)0)
#endif