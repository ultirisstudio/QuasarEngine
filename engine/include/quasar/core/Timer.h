#pragma once

/// \file
/// \brief A high-resolution (steady) timer for measuring elapsed time.
/// \author Fadiwahbeh
/// \date 2026-10-19

#include <chrono>

namespace Quasar
{
    /// @brief High-resolution (steady) timer that measures the time elapsed since it was created or last reset.
    ///
    /// It relies on a monotonic clock, so changes to the system time do not affect it.
    ///
    /// `Timer timer; LoadScene(); double ms = timer.ElapsedMilliseconds();`

    class Timer
    {
    public:
        /// @brief Clock used by the timer, monotonic with nanosecond resolution.
        ///
        /// `std::chrono::steady_clock` is not used on purpose: it is an alias whose target depends on the
        /// standard library, and with libstdc++ it is the system clock, which is not monotonic.
        using Clock = std::chrono::steady_clock;

        /// @brief Creates a timer that starts measuring right away.
        Timer() { Reset(); }

        /// @brief Resets the timer to zero.
        void Reset();

        /// @brief Returns the time elapsed since the timer was created or last reset, in seconds.
        /// @return The elapsed time in seconds.
        double ElapsedSeconds() const;

        /// @brief Returns the time elapsed since the timer was created or last reset, in milliseconds.
        /// @return The elapsed time in milliseconds.
        double ElapsedMilliseconds() const;

    private:
        Clock::time_point m_Start;
    };
}
