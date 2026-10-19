#pragma once

#include <chrono>

namespace Quasar
{
    class Timer
    {
    public:
        using Clock = std::chrono::high_resolution_clock;
        static_assert(Clock::is_steady, "Timer requires a monotonic clock");

        Timer() { Reset(); }

        void Reset();

        double ElapsedSeconds() const;
        double ElapsedMilliseconds() const;

    private:
        Clock::time_point m_Start;
    };
}
