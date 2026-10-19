#pragma once

/// \file
/// \brief Contains the Timestep class, which represents a time duration in seconds.
/// \author Kurogenshi
/// \date 2026-10-19

namespace Quasar
{
    /// @brief Time elapsed between two frames, in seconds.
    ///
    /// A Timestep converts to float implicitly, so it can be used directly in computations:
    /// `position += speed * timestep`.
    class Timestep
    {
    public:
        /// @brief Creates a Timestep of 0 seconds.
        constexpr Timestep() = default;

        /// @brief Creates a Timestep of the given number of seconds.
        /// @param seconds
        constexpr explicit Timestep(float seconds) : m_Seconds(seconds) {}

        /// @brief Returns the duration in seconds.
        constexpr float GetSeconds() const { return m_Seconds; }

        /// @brief Returns the duration in milliseconds.
        constexpr float GetMilliseconds() const { return m_Seconds * 1000.0f; }

        /// @brief Implicit conversion to float, returning the duration in seconds.
        constexpr operator float() const { return m_Seconds; }

    private:
        float m_Seconds = 0.0f;
    };
}
