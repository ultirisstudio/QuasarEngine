#include <quasar/core/UUID.h>

#include <doctest/doctest.h>

#include <cstddef>
#include <functional>
#include <thread>
#include <type_traits>
#include <unordered_set>
#include <vector>

static_assert(sizeof(Quasar::UUID) == 8, "A UUID must stay as small as a 64-bit integer");
static_assert(std::is_trivially_copyable_v<Quasar::UUID>, "A UUID must be copied like a plain integer");

namespace
{
    constexpr std::size_t UUIDsPerThread = 50'000;

    void GenerateUUIDs(std::vector<Quasar::UUID>& uuids)
    {
        uuids.reserve(UUIDsPerThread);
        for (std::size_t i = 0; i < UUIDsPerThread; ++i)
        {
            uuids.push_back(Quasar::UUID::Generate());
        }
    }
}

TEST_CASE("A default UUID is invalid")
{
    const Quasar::UUID uuid;

    CHECK_FALSE(uuid.IsValid());
    CHECK(uuid.GetValue() == 0);
}

TEST_CASE("A generated UUID is valid")
{
    const Quasar::UUID uuid = Quasar::UUID::Generate();

    CHECK(uuid.IsValid());
}

TEST_CASE("Generated UUIDs are unique")
{
    constexpr std::size_t count = 100'000;

    std::unordered_set<Quasar::UUID> uuids;
    for (std::size_t i = 0; i < count; ++i)
    {
        uuids.insert(Quasar::UUID::Generate());
    }

    CHECK(uuids.size() == count);
}

TEST_CASE("UUIDs generated on different threads are unique")
{
    std::vector<Quasar::UUID> first;
    std::vector<Quasar::UUID> second;
    {
        std::jthread firstThread(GenerateUUIDs, std::ref(first));
        std::jthread secondThread(GenerateUUIDs, std::ref(second));
    }

    std::unordered_set<Quasar::UUID> uuids(first.begin(), first.end());
    uuids.insert(second.begin(), second.end());

    CHECK(uuids.size() == 2 * UUIDsPerThread);
}

TEST_CASE("A UUID keeps its value when saved and loaded")
{
    const Quasar::UUID original = Quasar::UUID::Generate();

    const Quasar::UUID loaded(original.GetValue());

    CHECK(loaded == original);
}

TEST_CASE("UUIDs are compared by value")
{
    CHECK(Quasar::UUID(42) == Quasar::UUID(42));
    CHECK(Quasar::UUID(42) != Quasar::UUID(43));
    CHECK(Quasar::UUID(42) < Quasar::UUID(43));
}
