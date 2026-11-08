#include <quasar/time/Timer.h>

#include <doctest/doctest.h>

#include <chrono>
#include <thread>

TEST_CASE("A timer starts near zero and never goes back")
{
    const Quasar::Timer timer;

    const double first = timer.ElapsedSeconds();
    const double second = timer.ElapsedSeconds();

    CHECK(first >= 0.0);
    CHECK(second >= first);
}

TEST_CASE("A timer measures at least the time that passed")
{
    const Quasar::Timer timer;

    std::this_thread::sleep_for(std::chrono::milliseconds(20));

    CHECK(timer.ElapsedMilliseconds() >= 20.0);
    CHECK(timer.ElapsedSeconds() >= 0.020);
}

TEST_CASE("Resetting a timer starts it again from zero")
{
    Quasar::Timer timer;
    std::this_thread::sleep_for(std::chrono::milliseconds(50));
    const double beforeReset = timer.ElapsedMilliseconds();

    timer.Reset();

    CHECK(timer.ElapsedMilliseconds() < beforeReset);
}

TEST_CASE("Seconds and milliseconds give the same duration")
{
    const Quasar::Timer timer;
    std::this_thread::sleep_for(std::chrono::milliseconds(5));

    const double seconds = timer.ElapsedSeconds();
    const double milliseconds = timer.ElapsedMilliseconds();

    CHECK(milliseconds >= doctest::Approx(seconds * 1000.0));
    CHECK(milliseconds - seconds * 1000.0 < 100.0);
}
