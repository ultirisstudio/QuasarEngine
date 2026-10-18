#pragma once

#include <compare>
#include <cstddef>
#include <cstdint>
#include <functional>

namespace Quasar
{
    class UUID
    {
    public:
        constexpr UUID() = default;
        constexpr explicit UUID(std::uint64_t value) : m_Value(value) {}

        static UUID Generate();

        constexpr std::uint64_t GetValue() const { return m_Value; }
        constexpr bool IsValid() const { return m_Value != 0; }

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