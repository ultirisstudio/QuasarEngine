#pragma once

#include <cstdint>
#include <string>
#include <iosfwd>

namespace QuasarEngine
{
    class UUID {
    public:
        using value_type = std::uint64_t;

        UUID() noexcept;
        constexpr explicit UUID(value_type v) noexcept : m_UUID(v) {}
        UUID(const UUID&) = default;
        UUID& operator=(const UUID&) = default;

        static constexpr UUID Null() noexcept { return UUID(0); }

        constexpr value_type value() const noexcept { return m_UUID; }
        explicit constexpr operator value_type() const noexcept { return m_UUID; }

        constexpr bool operator==(const UUID& other) const noexcept { return m_UUID == other.m_UUID; }
        constexpr bool operator!=(const UUID& other) const noexcept { return m_UUID != other.m_UUID; }

        std::string ToString() const;

    private:
        value_type m_UUID{ 0 };
    };

    std::ostream& operator<<(std::ostream& os, const UUID& id);
}

namespace std
{
    template<>
    struct hash<QuasarEngine::UUID> {
        size_t operator()(const QuasarEngine::UUID& uuid) const noexcept {
            std::uint64_t v = uuid.value();
#if SIZE_MAX >= UINT64_MAX
            return static_cast<size_t>(v);
#else
            return static_cast<size_t>(v ^ (v >> 32));
#endif
        }
    };
}
