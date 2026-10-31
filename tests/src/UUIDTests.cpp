#include <quasar/core/UUID.h>

#include <doctest/doctest.h>
#include <spdlog/fmt/fmt.h>

#include <array>
#include <cstddef>
#include <functional>
#include <limits>
#include <optional>
#include <ostream> // CAPTURE prints a std::string_view with operator<<, which MSVC only declares without this header
#include <string>
#include <string_view>
#include <thread>
#include <type_traits>
#include <unordered_map>
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

TEST_CASE("Only the zero value is invalid")
{
    CHECK_FALSE(Quasar::UUID(0).IsValid());
    CHECK(Quasar::UUID(1).IsValid());
    CHECK(Quasar::UUID(std::numeric_limits<std::uint64_t>::max()).IsValid());
}

TEST_CASE("Reading sixteen zeros gives the invalid UUID")
{
    const std::optional<Quasar::UUID> read = Quasar::UUID::FromString("0000000000000000");

    REQUIRE(read.has_value());
    CHECK_FALSE(read->IsValid());
}

TEST_CASE("A generated UUID is valid")
{
    const Quasar::UUID uuid = Quasar::UUID::Generate();

    CHECK(uuid.IsValid());
}

TEST_CASE("Generated UUIDs are unique and all valid")
{
    constexpr std::size_t count = 100'000;

    std::unordered_set<Quasar::UUID> uuids;
    std::size_t invalidCount = 0;
    for (std::size_t i = 0; i < count; ++i)
    {
        const Quasar::UUID uuid = Quasar::UUID::Generate();
        if (!uuid.IsValid())
        {
            ++invalidCount;
        }
        uuids.insert(uuid);
    }

    CHECK(invalidCount == 0);
    CHECK(uuids.size() == count);
}

TEST_CASE("UUIDs generated on different threads are unique")
{
    std::vector<Quasar::UUID> first;
    std::vector<Quasar::UUID> second;
    {
        const std::jthread firstThread(GenerateUUIDs, std::ref(first));
        const std::jthread secondThread(GenerateUUIDs, std::ref(second));
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

TEST_CASE("Equal UUIDs have the same hash")
{
    const Quasar::UUID uuid = Quasar::UUID::Generate();
    const std::hash<Quasar::UUID> hash;

    CHECK(hash(uuid) == hash(Quasar::UUID(uuid.GetValue())));
    CHECK(hash(Quasar::UUID()) == hash(Quasar::UUID(0)));
}

TEST_CASE("UUIDs work as keys of an unordered map")
{
    const Quasar::UUID first = Quasar::UUID::Generate();
    const Quasar::UUID second = Quasar::UUID::Generate();

    std::unordered_map<Quasar::UUID, std::string> names;
    names[first] = "Camera";
    names[second] = "Light";

    CHECK(names.size() == 2);
    CHECK(names.at(first) == "Camera");
    CHECK(names.at(second) == "Light");
    CHECK(names.find(Quasar::UUID(first.GetValue())) != names.end());
}

TEST_CASE("A UUID is written as 16 lowercase hexadecimal digits")
{
    CHECK(Quasar::UUID(0xAB).ToString() == "00000000000000ab");
    CHECK(Quasar::UUID(0x0123456789ABCDEF).ToString() == "0123456789abcdef");
    CHECK(Quasar::UUID().ToString() == "0000000000000000");
}

TEST_CASE("Converting a UUID to text and back gives the same value")
{
    const auto uuids = std::to_array<Quasar::UUID>({Quasar::UUID::Generate(), Quasar::UUID(), Quasar::UUID(1),
                                                    Quasar::UUID(std::numeric_limits<std::uint64_t>::max())});

    for (const Quasar::UUID uuid : uuids)
    {
        const std::optional<Quasar::UUID> read = Quasar::UUID::FromString(uuid.ToString());

        REQUIRE(read.has_value());
        CHECK(*read == uuid);
    }
}

TEST_CASE("Uppercase hexadecimal digits are read too")
{
    const std::optional<Quasar::UUID> read = Quasar::UUID::FromString("0123456789ABCDEF");

    REQUIRE(read.has_value());
    CHECK(read->GetValue() == 0x0123456789ABCDEF);
}

TEST_CASE("Invalid text gives no UUID")
{
    const auto invalidTexts = std::to_array<std::string_view>({
        "",                  // empty
        "123",               // too short
        "00000000000000001", // too long
        "0x000000000000ab",  // prefix
        "zzzzzzzzzzzzzzzz",  // not hexadecimal
        " 00000000000000a",  // space before
        "00000000000000a ",  // space after
        "-000000000000001",  // sign
        "+000000000000001",  // sign
    });

    for (const std::string_view text : invalidTexts)
    {
        CAPTURE(text);
        CHECK_FALSE(Quasar::UUID::FromString(text).has_value());
    }
}

TEST_CASE("A UUID is printed by the log macros as text")
{
    CHECK(fmt::format("{}", Quasar::UUID(0xAB)) == "00000000000000ab");
}
