#include "LogHelpers.h"

#include <quasar/core/MemoryLogSink.h>
#include <quasar/io/UserDataDirectory.h>

#include <doctest/doctest.h>

#include <filesystem>
#include <optional>

namespace
{
    constexpr const char* TestOrganization = "Quasar Engine";
    constexpr const char* TestApplication = "Quasar Engine Tests";
}

TEST_CASE("The user data directory is an absolute folder of the application inside the folder of the organization")
{
    const std::optional<std::filesystem::path> directory = Quasar::GetUserDataDirectory(TestOrganization, TestApplication);

    REQUIRE(directory.has_value());
    CHECK(directory->is_absolute());
    CHECK(directory->filename() == TestApplication);
    CHECK(directory->parent_path().filename() == TestOrganization);
    CHECK(std::filesystem::is_directory(*directory));
}

TEST_CASE("The user data directory is the same on every call")
{
    CHECK(Quasar::GetUserDataDirectory(TestOrganization, TestApplication) == Quasar::GetUserDataDirectory(TestOrganization, TestApplication));
}

TEST_CASE("Names with letters, digits, spaces and UTF-8 letters are valid")
{
    CHECK(Quasar::IsValidUserDataName("Quasar Engine"));
    CHECK(Quasar::IsValidUserDataName("Star Rider 2"));
    CHECK(Quasar::IsValidUserDataName("\xC3\x89quipe"));
}

TEST_CASE("Empty names, names with punctuation and names that start or end with a space are not valid")
{
    CHECK_FALSE(Quasar::IsValidUserDataName(""));
    CHECK_FALSE(Quasar::IsValidUserDataName("Game 2: Revenge"));
    CHECK_FALSE(Quasar::IsValidUserDataName("../Other"));
    CHECK_FALSE(Quasar::IsValidUserDataName("Folder\\Game"));
    CHECK_FALSE(Quasar::IsValidUserDataName(" Game"));
    CHECK_FALSE(Quasar::IsValidUserDataName("Game "));
}

TEST_CASE("No user data directory is given for a name that is not valid")
{
    const TestHelpers::ScopedLog log;

    CHECK_FALSE(Quasar::GetUserDataDirectory("", TestApplication).has_value());
    CHECK_FALSE(Quasar::GetUserDataDirectory(TestOrganization, "../Other").has_value());
    CHECK(TestHelpers::HasLogAtLeast(Quasar::LogLevel::Error));
}
