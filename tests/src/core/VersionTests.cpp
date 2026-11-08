#include <quasar/Version.h>

#include <doctest/doctest.h>

#include <string>

TEST_CASE("The engine version matches the project version")
{
    const std::string version = Quasar::GetVersionString();

    CHECK_FALSE(version.empty());
    CHECK(version == QUASAR_TEST_EXPECTED_VERSION);
}
