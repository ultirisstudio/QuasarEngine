#pragma once

/// \file
/// \brief Timer that logs how long a scope took when the scope ends.
/// \author Fadiwahbeh
/// \date 2026-10-23

#include <quasar/core/Timer.h>

#include <string>

namespace Quasar
{
    /// @brief Measures the time between its creation and the end of its scope, then logs it as an engine info
    /// message.
    ///
    /// The timer starts in the constructor and the time is logged in the destructor, so the scope does the work.
    /// The message has the form `<name> took <time> ms`:
    ///
    /// `{ ScopedTimer timer("LoadScene"); LoadScene(); }`
    ///
    /// Log::Init() must have been called before the scope ends.
    class ScopedTimer
    {
    public:
        /// @brief Starts measuring right away.
        /// @param name Name written in the log message, to know which scope was measured.
        explicit ScopedTimer(std::string name);

        /// @brief Logs the name and the time elapsed since the creation, in milliseconds.
        ~ScopedTimer();

        ScopedTimer(const ScopedTimer&) = delete;
        ScopedTimer& operator=(const ScopedTimer&) = delete;
        ScopedTimer(ScopedTimer&&) = delete;
        ScopedTimer& operator=(ScopedTimer&&) = delete;

    private:
        std::string m_Name;
        Timer m_Timer;
    };
}
