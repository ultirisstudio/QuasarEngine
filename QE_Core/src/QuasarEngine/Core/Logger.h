#pragma once

#include <iostream>
#include <fstream>
#include <string>
#include <ctime>
#include <sstream>

class Logger {
public:
    enum class Level {
        INFO_LOG,
        WARNING_LOG,
        ERROR_LOG,
        DEBUG_LOG
    };

    Logger(const std::string& filename = "log.txt") : logFile(filename, std::ios::app) {}

    ~Logger() {
        if (logFile.is_open()) {
            logFile.close();
        }
    }

    void log(Level level, const std::string& message) {
        if (!logFile.is_open()) return;

        std::string levelStr = levelToString(level);
        std::string timestamp = getCurrentTimestamp();

        std::stringstream logMessage;
        logMessage << "[" << timestamp << "] [" << levelStr << "] " << message << std::endl;

        std::cout << logMessage.str();

        logFile << logMessage.str();
    }

private:
    std::ofstream logFile;

    std::string levelToString(Level level) {
        switch (level) {
        case Level::INFO_LOG: return "INFO";
        case Level::WARNING_LOG: return "WARNING";
        case Level::ERROR_LOG: return "ERROR";
        case Level::DEBUG_LOG: return "DEBUG";
        default: return "UNKNOWN";
        }
    }

    std::string getCurrentTimestamp() {
        std::time_t now = std::time(0);
        std::tm* tm_info = std::localtime(&now);
        std::stringstream ss;
        ss << (tm_info->tm_year + 1900) << "-"
            << (tm_info->tm_mon + 1) << "-"
            << tm_info->tm_mday << " "
            << tm_info->tm_hour << ":"
            << tm_info->tm_min << ":"
            << tm_info->tm_sec;
        return ss.str();
    }
};