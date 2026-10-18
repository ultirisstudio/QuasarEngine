#include <quasar/core/Timestep.h>

#include <doctest/doctest.h>

#include <type_traits>

static_assert(sizeof(Quasar::Timestep) == sizeof(float), "A Timestep must stay as small as a float");
static_assert(std::is_trivially_copyable_v<Quasar::Timestep>, "A Timestep must be copied like a plain float");
static_assert(Quasar::Timestep(0.5f).GetMilliseconds() == 500.0f, "The conversion must work at compile time");

TEST_CASE("A default Timestep is zero")
{
    const Quasar::Timestep timestep;

    CHECK(timestep.GetSeconds() == 0.0f);
    CHECK(timestep.GetMilliseconds() == 0.0f);
}

TEST_CASE("A Timestep converts to seconds and milliseconds")
{
    const Quasar::Timestep timestep(0.25f);

    CHECK(timestep.GetSeconds() == 0.25f);
    CHECK(timestep.GetMilliseconds() == 250.0f);
}

TEST_CASE("A frame time at 60 frames per second is about 16.7 milliseconds")
{
    const Quasar::Timestep timestep(1.0f / 60.0f);

    CHECK(timestep.GetMilliseconds() == doctest::Approx(16.667).epsilon(0.001));
}

TEST_CASE("A Timestep can be used directly in a calculation")
{
    const Quasar::Timestep timestep(0.25f);
    const float speed = 4.0f;

    const float distance = speed * timestep;

    CHECK(distance == 1.0f);
}