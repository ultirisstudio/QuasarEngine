#include "qepch.h"

#include <iostream>
#include <cstdio>
#include <ctime>
#include <chrono>
#include <sstream>
#include <fstream>
#include <iomanip>
#include <thread>
#include <clocale>
#include <locale>

#include <QuasarEngine/Core/Logger.h>

#if defined(_WIN32)
#include <io.h>
#define ISATTY _isatty
#define FILENO _fileno
#else
#include <unistd.h>
#define ISATTY isatty
#define FILENO fileno
#endif

namespace QuasarEngine
{
    std::mutex& Logger::mutex_() {
        static std::mutex m;
        return m;
    }

    std::vector<Logger::Sink>& Logger::sinks_() {
        static std::vector<Sink> v;
        return v;
    }

    std::atomic<int32_t>& Logger::minLevel_() {
        static std::atomic<int32_t> v{ static_cast<int32_t>(Logger::Level::Debug) };
        return v;
    }

    std::atomic<bool>& Logger::abortOnFatal_() {
#if defined(NDEBUG)
        static std::atomic<bool> v{ false };
#else
        static std::atomic<bool> v{ true };
#endif
        return v;
    }

    std::atomic<bool>& Logger::colors_() {
        static std::atomic<bool> v{ true };
        return v;
    }

    static inline const char* ANSI_RESET = "\x1b[0m";
    static inline const char* ANSI_BOLD = "\x1b[1m";
    static inline const char* ANSI_WHITE = "\x1b[37m";
    static inline const char* ANSI_BWHITE = "\x1b[1;37m";
    static inline const char* ANSI_YELLOW = "\x1b[33m";
    static inline const char* ANSI_BYELLOW = "\x1b[1;33m";
    static inline const char* ANSI_RED = "\x1b[31m";
    static inline const char* ANSI_BRED = "\x1b[1;31m";
    static inline const char* ANSI_CYAN = "\x1b[36m";
    static inline const char* ANSI_BCYAN = "\x1b[1;36m";

    const char* Logger::levelColor(Level level) noexcept {
        if (!colorsEnabled()) return "";
        switch (level) {
        case Level::Debug:   return ANSI_CYAN;
        case Level::Info:    return ANSI_BWHITE;
        case Level::Warning: return ANSI_BYELLOW;
        case Level::Error:   return ANSI_BRED;
        case Level::Fatal:   return ANSI_BRED;
        default:             return "";
        }
    }

    std::tm Logger::localtimeSafe(std::time_t t) noexcept {
        std::tm tm{};
#if defined(_WIN32)
        localtime_s(&tm, &t);
#else
        localtime_r(&t, &tm);
#endif
        return tm;
    }

    bool Logger::isTerminal() noexcept {
        std::ostream* os = nullptr;
        {
            std::lock_guard<std::mutex> lock(mutex_());
            os = sinks_().empty() ? &std::cout : sinks_().front().stream;
        }

        FILE* f = stdout;
        if (os == &std::cerr) f = stderr;
#if defined(_WIN32)
        return ISATTY(FILENO(f)) != 0;
#else
        return ISATTY(FILENO(f)) == 1;
#endif
    }

    void Logger::tryEnableVTOnWindows() noexcept {
#if defined(_WIN32)
        HANDLE hOut = GetStdHandle(STD_OUTPUT_HANDLE);
        if (hOut == INVALID_HANDLE_VALUE) return;
        DWORD mode = 0;
        if (!GetConsoleMode(hOut, &mode)) return;
        mode |= ENABLE_VIRTUAL_TERMINAL_PROCESSING;
        SetConsoleMode(hOut, mode);
#endif
    }

    void Logger::setMinLevel(Level lvl) noexcept { minLevel_().store(static_cast<int32_t>(lvl), std::memory_order_relaxed); }
    Logger::Level Logger::minLevel() noexcept { return static_cast<Level>(minLevel_().load(std::memory_order_relaxed)); }

    void Logger::enableColors(bool enabled) noexcept { colors_().store(enabled, std::memory_order_relaxed); }
    bool Logger::colorsEnabled() noexcept {
        static std::once_flag once;
        std::call_once(once, [] {
            tryEnableVTOnWindows();
            if (!isTerminal()) {
                colors_().store(false, std::memory_order_relaxed);
            }
            });
        return colors_().load(std::memory_order_relaxed);
    }

    void Logger::setAbortOnFatal(bool enabled) noexcept { abortOnFatal_().store(enabled, std::memory_order_relaxed); }
    bool Logger::abortOnFatal() noexcept { return abortOnFatal_().load(std::memory_order_relaxed); }

    void Logger::addSink(std::ostream& os) {
        std::lock_guard<std::mutex> lock(mutex_());
        sinks_().push_back(Sink{ &os, false });
    }

    bool Logger::addFileSink(const std::string& path, bool truncate) {
        auto fs = new std::ofstream();
        fs->open(path, truncate ? (std::ios::out | std::ios::trunc) : (std::ios::out | std::ios::app));
        if (!*fs) { delete fs; return false; }
        std::lock_guard<std::mutex> lock(mutex_());
        sinks_().push_back(Sink{ fs, true });
        return true;
    }

    void Logger::clearSinks() {
        std::lock_guard<std::mutex> lock(mutex_());
        for (auto& s : sinks_()) {
            if (s.owned) { delete s.stream; }
        }
        sinks_().clear();
    }

    void Logger::initUtf8Console() noexcept {
#if defined(_WIN32)
        SetConsoleOutputCP(CP_UTF8);
        SetConsoleCP(CP_UTF8);

        HANDLE hOut = GetStdHandle(STD_OUTPUT_HANDLE);
        if (hOut != INVALID_HANDLE_VALUE) {
            DWORD mode = 0;
            if (GetConsoleMode(hOut, &mode)) {
                mode |= ENABLE_VIRTUAL_TERMINAL_PROCESSING | ENABLE_PROCESSED_OUTPUT;
                SetConsoleMode(hOut, mode);
            }
        }
#endif
        std::setlocale(LC_ALL, "");
        try { std::locale::global(std::locale("")); }
        catch (...) {}
    }


    bool Logger::shouldLog(Level level) noexcept {
        return static_cast<int32_t>(level) >= minLevel_().load(std::memory_order_relaxed);
    }

    std::string Logger::makeHeader(Level level, const char* file, int line) noexcept {
        using namespace std::chrono;
        const auto now = system_clock::now();
        const auto tt = system_clock::to_time_t(now);
        const auto tm = localtimeSafe(tt);

        const auto ms = duration_cast<milliseconds>(now.time_since_epoch()).count() % 1000;

        std::ostringstream oss;

        oss << '[' << std::put_time(&tm, "%Y-%m-%d %H:%M:%S")
            << '.' << std::setw(3) << std::setfill('0') << ms << "] ";

        if (colorsEnabled()) oss << levelColor(level);
        oss << '[' << levelToString(level) << ']';
        if (colorsEnabled()) oss << ANSI_RESET;
        oss << ' ';

#if Q_LOG_WITH_THREAD_ID
        std::ostringstream tid;
        tid << std::this_thread::get_id();
        oss << '[' << tid.str() << "] ";
#endif

#if Q_LOG_SHORT_FILE
        const char* fname = file;
        for (const char* p = file; *p; ++p) {
            if (*p == '/' || *p == '\\') fname = p + 1;
        }
#else
        const char* fname = file;
#endif
        if (colorsEnabled()) oss << ANSI_BOLD << ANSI_WHITE;
        oss << '[' << fname << ':' << line << ']';
        if (colorsEnabled()) oss << ANSI_RESET;

        oss << ' ';
        return oss.str();
    }

    void Logger::writeLine(Level level, std::string_view header, std::string_view message) noexcept {
        std::lock_guard<std::mutex> lock(mutex_());
        if (sinks_().empty()) {
            (level >= Level::Error ? std::cerr : std::cout) << header << message << '\n';
            return;
        }

        for (auto& s : sinks_()) {
            if (!s.stream) continue;
            *s.stream << header << message << '\n';
            s.stream->flush();
        }
    }

    void Logger::log(Level level, std::string_view message, const char* file, int line) noexcept {
        if (!shouldLog(level)) return;
        const auto header = makeHeader(level, file, line);
        writeLine(level, header, message);

        if (level == Level::Fatal && abortOnFatal()) {
            std::lock_guard<std::mutex> lock(mutex_());
            for (auto& s : sinks_()) { if (s.stream) s.stream->flush(); }
            std::abort();
        }
    }

    void Logger::checkAssert(bool condition, std::string_view message, const char* file, int line) noexcept {
        if (!condition) {
            log(Level::Fatal, message, file, line);
            std::exit(EXIT_FAILURE);
        }
    }

    void Logger::apiInfo(std::string_view apiName,
        std::initializer_list<std::pair<std::string_view, std::string_view>> fields) noexcept {
        std::ostringstream oss;
        oss << "\n========= " << apiName << " Info =========\n";
        for (auto&& kv : fields) {
            oss << std::setw(10) << std::left << kv.first << ": " << kv.second << "\n";
        }
        oss << "=====================================\n";
        writeLine(Level::Info, "", oss.str());
    }
}
