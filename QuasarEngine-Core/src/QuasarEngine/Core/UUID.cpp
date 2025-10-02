#include "qepch.h"

#include "UUID.h"

#include <random>
#include <sstream>
#include <limits>
#include <mutex>

namespace
{
    std::mt19937_64& engine() {
        static std::mt19937_64 e([] {
            std::random_device rd;
            std::seed_seq seed{ rd(), rd(), rd(), rd(), rd(), rd(), rd(), rd() };
            return std::mt19937_64(seed);
            }());
        return e;
    }

    std::uniform_int_distribution<std::uint64_t>& distrib() {
        static std::uniform_int_distribution<std::uint64_t> d(
            std::numeric_limits<std::uint64_t>::min(),
            std::numeric_limits<std::uint64_t>::max()
        );
        return d;
    }

    std::mutex& rngMutex() {
        static std::mutex m;
        return m;
    }
}

namespace QuasarEngine
{
    UUID::UUID() noexcept {
        std::lock_guard<std::mutex> lock(rngMutex());
        m_UUID = distrib()(engine());
    }

    std::string UUID::ToString() const {
        return std::to_string(m_UUID);
    }

    std::ostream& operator<<(std::ostream& os, const UUID& id) {
        return (os << id.value());
    }
}
