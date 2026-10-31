#pragma once

/// \file
/// \brief Timer that logs how long a scope took when the scope ends.
/// \author Fadiwahbeh
/// \date 2026-10-23

#include <quasar/core/Base.h>
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
    /// To measure a scope in Debug only, use QUASAR_SCOPED_TIMER() instead of creating a ScopedTimer directly.
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

/// \def QUASAR_SCOPED_TIMER(name)
/// @brief Measures the rest of the current scope and logs the time with the given name. Removed from Release
/// builds, where `name` is still checked by the compiler but never evaluated.
///
/// `{ QUASAR_SCOPED_TIMER("LoadScene"); LoadScene(); }`
///
/// The macro creates a ScopedTimer with a variable name that contains the line number, so several timers can be
/// used in the same scope, one per line. Because `name` is still compiled in Release, a variable used only as the
/// name does not cause an unused variable warning.
#ifdef QUASAR_DEBUG
#define QUASAR_SCOPED_TIMER(name) const ::Quasar::ScopedTimer QUASAR_CONCAT(scopedTimer, __LINE__)(name)
#else
#define QUASAR_SCOPED_TIMER(name)                                                                                      \
    do                                                                                                                 \
    {                                                                                                                  \
        if constexpr (false)                                                                                           \
        {                                                                                                              \
            static_cast<void>(::Quasar::ScopedTimer(name));                                                            \
        }                                                                                                              \
    } while (false)
#endif
