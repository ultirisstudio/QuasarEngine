#include <quasar/core/UUID.h>

#include <spdlog/fmt/fmt.h>

#include <charconv>
#include <cstddef>
#include <limits>
#include <random>

namespace Quasar
{
    namespace
    {
        std::mt19937_64 CreateEngine()
        {
            std::random_device device;
            std::seed_seq seed{device(), device(), device(), device(), device(), device(), device(), device()};
            return std::mt19937_64(seed);
        }
    }

    UUID UUID::Generate()
    {
        thread_local std::mt19937_64 engine = CreateEngine();

        std::uniform_int_distribution<std::uint64_t> distribution(1, std::numeric_limits<std::uint64_t>::max());
        return UUID(distribution(engine));
    }

    std::string UUID::ToString() const
    {
        return fmt::format("{:016x}", m_Value);
    }

    std::optional<UUID> UUID::FromString(std::string_view text)
    {
        constexpr std::size_t digitCount = 16;
        if (text.size() != digitCount)
        {
            return std::nullopt;
        }

        std::uint64_t value = 0;
        const char* end = text.data() + text.size();
        const auto [last, error] = std::from_chars(text.data(), end, value, 16);
        if (error != std::errc{} || last != end)
        {
            return std::nullopt;
        }

        return UUID(value);
    }
}
