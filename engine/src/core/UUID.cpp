#include <quasar/core/UUID.h>

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
}