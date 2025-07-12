#include "qepch.h"
#include "Logger.h"

#include <iostream>
#include <chrono>
#include <iomanip>
#include <sstream>
#include <cstdlib>

#define RESET		"\033[0m"
#define BLACK		"\033[30m"
#define RED			"\033[31m"
#define GREEN		"\033[32m"
#define YELLOW		"\033[33m"
#define BLUE		"\033[34m"
#define MAGENTA		"\033[35m"
#define CYAN		"\033[36m"
#define WHITE		"\033[37m"
#define BOLDBLACK   "\033[1m\033[30m"
#define BOLDRED     "\033[1m\033[31m"
#define BOLDGREEN   "\033[1m\033[32m"
#define BOLDYELLOW  "\033[1m\033[33m"
#define BOLDBLUE    "\033[1m\033[34m"
#define BOLDMAGENTA "\033[1m\033[35m"
#define BOLDCYAN    "\033[1m\033[36m"
#define BOLDWHITE   "\033[1m\033[37m"

namespace QuasarEngine
{
    void Logger::log(Level level, const std::string& message, const char* file, int line)
    {
        printHeader(level, file, line);
        printColored(level, message);
    }

    void Logger::checkAssert(bool condition, const std::string& message, const char* file, int line)
    {
        if (!condition)
        {
            log(Level::Fatal, message, file, line);
            std::exit(EXIT_FAILURE);
        }
    }

    void Logger::apiInfo(const std::string& vendor, const std::string& renderer, const std::string& version)
    {
        std::cout << "\n======== Vulkan API Info ========\n";
        std::cout << "Vendor   : " << vendor << "\n";
        std::cout << "Renderer : " << renderer << "\n";
        std::cout << "Version  : " << version << "\n";
        std::cout << "=================================\n\n";
    }

    void Logger::printHeader(Level level, const char* file, int line)
    {
        std::string time = getCurrentTime();

        const char* filename = std::strrchr(file, '/');
        if (!filename) filename = std::strrchr(file, '\\');
        filename = (filename ? filename + 1 : file);

        std::cout
            << YELLOW << "[" << time << "] "
            << levelColor(level) << "[" << levelToString(level) << "] "
            << BOLDWHITE << "[" << filename << ":" << line << "] "
            << RESET;
    }

    void Logger::printColored(Level level, const std::string& message)
    {
        std::cout << WHITE << message << RESET << std::endl;
    }

    const char* Logger::levelToString(Level level)
    {
        switch (level)
        {
        case Level::Info:    return "INFO";
        case Level::Warning: return "WARNING";
        case Level::Error:   return "ERROR";
        case Level::Fatal:   return "FATAL";
        case Level::Debug:   return "DEBUG";
        default:             return "UNKNOWN";
        }
    }

    const char* Logger::levelColor(Level level)
    {
        switch (level)
        {
        case Level::Info:    return BOLDWHITE;
        case Level::Warning: return BOLDYELLOW;
        case Level::Error:   return BOLDRED;
        case Level::Fatal:   return BOLDRED;
        case Level::Debug:   return CYAN;
        default:             return RESET;
        }
    }

    std::string Logger::getCurrentTime()
    {
        using namespace std::chrono;
        auto now = system_clock::now();
        auto time = system_clock::to_time_t(now);
        std::stringstream ss;
        ss << std::put_time(std::localtime(&time), "%Y-%m-%d %H:%M:%S");
        return ss.str();
    }
}
