#pragma once

/// \file
/// \brief Unique identifier for entities, assets and any object saved in a file.
/// \author Kurogenshi
/// \date 2026-10-19

#include <compare>
#include <cstddef>
#include <cstdint>
#include <functional>

namespace Quasar
{
    /// @brief Permanent identifier for entities, assets and any object that must keep the same identity across
    /// sessions.
    ///
    /// The value is saved as is in scene and asset files. The value 0 is reserved: a default-constructed UUID is
    /// invalid. Use Generate() to create a new identifier. A UUID can be used as a key in `std::map` and
    /// `std::unordered_map`.
    class UUID
    {
    public:
        /// @brief Creates an invalid UUID, with the value 0.
        constexpr UUID() = default;

        /// @brief Creates a UUID from a saved value, for example when a scene is loaded.
        /// @param value The value returned by GetValue() when the UUID was saved.
        constexpr explicit UUID(std::uint64_t value) : m_Value(value) {}

        /// @brief Creates a new random UUID. Safe to call from any thread.
        /// @return A valid UUID, never equal to 0.
        static UUID Generate();

        /// @brief Returns the raw value, to save the UUID in a file.
        /// @return The 64-bit value of the UUID.
        constexpr std::uint64_t GetValue() const { return m_Value; }

        /// @brief Returns true if the UUID is not the reserved value 0.
        /// @return False for a default-constructed UUID, true for a generated one.
        constexpr bool IsValid() const { return m_Value != 0; }

        /// @brief Compares two UUIDs by their value, so they can be sorted and used as map keys.
        constexpr auto operator<=>(const UUID&) const = default;

    private:
        std::uint64_t m_Value = 0;
    };
}

namespace std
{
    template <>
    struct hash<Quasar::UUID>
    {
        std::size_t operator()(const Quasar::UUID& uuid) const noexcept
        {
            return std::hash<std::uint64_t>{}(uuid.GetValue());
        }
    };
}
