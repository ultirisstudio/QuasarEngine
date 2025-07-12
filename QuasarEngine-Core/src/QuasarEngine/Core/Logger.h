#pragma once

#include <string>

namespace QuasarEngine
{
    class Logger
    {
    public:
        enum class Level {
            Info,
            Warning,
            Error,
            Fatal,
            Debug
        };

        static void log(Level level, const std::string& message, const char* file, int line);
        static void apiInfo(const std::string& vendor, const std::string& renderer, const std::string& version);
        static void checkAssert(bool condition, const std::string& message, const char* file, int line);

    private:
        Logger() = delete;

        static void printColored(Level level, const std::string& message);
        static void printHeader(Level level, const char* file, int line);
        static const char* levelToString(Level level);
        static const char* levelColor(Level level);
        static std::string getCurrentTime();
    };
}

#define Q_ASSERT(cond, msg) ::QuasarEngine::Logger::checkAssert(cond, msg, __FILE__, __LINE__)
#define Q_INFO(msg)         ::QuasarEngine::Logger::log(::QuasarEngine::Logger::Level::Info, msg, __FILE__, __LINE__)
#define Q_WARNING(msg)      ::QuasarEngine::Logger::log(::QuasarEngine::Logger::Level::Warning, msg, __FILE__, __LINE__)
#define Q_ERROR(msg)        ::QuasarEngine::Logger::log(::QuasarEngine::Logger::Level::Error, msg, __FILE__, __LINE__)
#define Q_FATAL(msg)        ::QuasarEngine::Logger::log(::QuasarEngine::Logger::Level::Fatal, msg, __FILE__, __LINE__)

#ifdef DEBUG
#define Q_DEBUG(msg)    ::QuasarEngine::Logger::log(::QuasarEngine::Logger::Level::Debug, msg, __FILE__, __LINE__)
#else
#define Q_DEBUG(msg)    ((void)0)
#endif
