#pragma once

/// \file
/// \brief A point in the work of the graphics card, to measure how long its commands take.
/// \author Kurogenshi
/// \date 2026-11-24

#include <cstdint>
#include <optional>

namespace Quasar
{
    /// @brief Reads the clock of the graphics card when it reaches a point between two commands.
    ///
    /// The graphics card runs the commands later than the program sends them, so the time is only known a few frames
    /// after Record(). GetNanoseconds() never waits for it: it gives no value until the time is ready. A query is
    /// created with GraphicsContext::CreateTimestampQuery(), and FrameProfiler uses two of them for each section.
    class TimestampQuery
    {
    public:
        virtual ~TimestampQuery() = default;

        TimestampQuery(const TimestampQuery&) = delete;
        TimestampQuery& operator=(const TimestampQuery&) = delete;
        TimestampQuery(TimestampQuery&&) = delete;
        TimestampQuery& operator=(TimestampQuery&&) = delete;

        /// @brief Asks for the time at which the graphics card finishes the commands sent before this call. A new call
        /// replaces the previous time.
        virtual void Record() = 0;

        /// @brief Returns the time of the last Record(), without waiting for the graphics card.
        /// @return The time in nanoseconds, from a start that only matters to compare two queries, or no value if
        /// the graphics card has not reached the point yet or if Record() was never called.
        [[nodiscard]] virtual std::optional<std::uint64_t> GetNanoseconds() = 0;

    protected:
        TimestampQuery() = default;
    };
}
