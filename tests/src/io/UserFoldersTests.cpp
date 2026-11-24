#include "helpers/LogHelpers.h"

#include <quasar/io/UserFolders.h>
#include <quasar/log/MemoryLogSink.h>

#include <doctest/doctest.h>

#include <filesystem>
#include <optional>

TEST_CASE("The home folder of the user is an absolute folder that exists")
{
    const TestHelpers::ScopedLog log;

    const std::optional<std::filesystem::path> home = Quasar::GetUserFolder(Quasar::UserFolder::Home);

    REQUIRE(home.has_value());
    CHECK(home->is_absolute());
    CHECK(home->has_filename());
    CHECK(std::filesystem::is_directory(*home));
}

TEST_CASE("The documents folder of the user is absolute when the system gives one, and its absence is not an error")
{
    const TestHelpers::ScopedLog log;

    const std::optional<std::filesystem::path> documents = Quasar::GetUserFolder(Quasar::UserFolder::Documents);

    if (documents.has_value())
    {
        CHECK(documents->is_absolute());
        CHECK(documents->has_filename());
    }
    CHECK_FALSE(TestHelpers::HasLogAtLeast(Quasar::LogLevel::Warning));
}
