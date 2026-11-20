#include <Project.h>

#include "helpers/LogHelpers.h"
#include "helpers/TemporaryFiles.h"

#include <quasar/io/File.h>
#include <quasar/io/Path.h>

#include <doctest/doctest.h>

#include <filesystem>
#include <string>

using Quasar::PathFromUtf8;
using Quasar::Editor::CreateProject;
using Quasar::Editor::GetAssetsDirectory;
using Quasar::Editor::GetDefaultProjectLocation;
using Quasar::Editor::OpenProject;
using Quasar::Editor::PathToUtf8;
using Quasar::Editor::ProjectResult;
using Quasar::Editor::ResolveProjectDirectory;

namespace
{
    void CheckFailed(const ProjectResult& result)
    {
        CHECK_FALSE(result.IsValid());
        CHECK_FALSE(result.Error.empty());
        CHECK(result.Directory.empty());
    }
}

TEST_SUITE("Project")
{
    TEST_CASE("PathToUtf8 gives back the UTF-8 text of a path made by PathFromUtf8")
    {
        const std::string text = "Proj\xC3\xA9t \xE6\x97\xA5\xE6\x9C\xAC";

        CHECK(PathToUtf8(PathFromUtf8(text)) == text);
    }

    TEST_CASE("GetAssetsDirectory points to the Assets folder inside the project folder")
    {
        const std::filesystem::path project = std::filesystem::temp_directory_path() / "MyGame";

        const std::filesystem::path assets = GetAssetsDirectory(project);

        CHECK(assets.filename() == "Assets");
        CHECK(assets.parent_path() == project);
    }

    TEST_CASE("GetDefaultProjectLocation is a full path that ends with the Quasar Projects folder")
    {
        const std::filesystem::path location = GetDefaultProjectLocation();

        REQUIRE_FALSE(location.empty());
        CHECK(location.is_absolute());
        CHECK(location.filename() == "Quasar Projects");
    }

    TEST_CASE("ResolveProjectDirectory gives the normalized full path of the project folder")
    {
        const TestHelpers::TemporaryFolder folder("quasar_resolve_project");
        const std::filesystem::path expected = (folder.GetPath() / "MyGame").lexically_normal();

        SUBCASE("from a plain location")
        {
            CHECK(ResolveProjectDirectory(folder.GetPath(), "MyGame") == expected);
        }
        SUBCASE("from a location with dot segments")
        {
            CHECK(ResolveProjectDirectory(folder.GetPath() / "Sub" / ".." / ".", "MyGame") == expected);
        }
        SUBCASE("from a location that does not exist yet")
        {
            const std::filesystem::path missing = folder.GetPath() / "Missing";
            CHECK(ResolveProjectDirectory(missing, "MyGame") == (missing / "MyGame").lexically_normal());
        }
    }

    TEST_CASE("ResolveProjectDirectory refuses a location that is not a full path")
    {
        CHECK(ResolveProjectDirectory("", "MyGame").empty());
        CHECK(ResolveProjectDirectory("Projects", "MyGame").empty());
        CHECK(ResolveProjectDirectory(std::filesystem::path(".") / "Projects", "MyGame").empty());
    }

    TEST_CASE("ResolveProjectDirectory refuses a name that cannot be a folder name")
    {
        const TestHelpers::TemporaryFolder folder("quasar_resolve_name");

        for (const char* name : {"", ".", "..", "My/Game", "My\\Game", "Game.", "Game ", "Ga:me", "Ga*me", "Tab\tName"})
        {
            CAPTURE(name);
            CHECK(ResolveProjectDirectory(folder.GetPath(), name).empty());
        }
    }

    TEST_CASE("CreateProject creates an empty Assets folder inside the new project folder")
    {
        const TestHelpers::TemporaryFolder folder("quasar_create_project");

        const ProjectResult result = CreateProject(folder.GetPath(), "MyGame");

        REQUIRE_MESSAGE(result.IsValid(), result.Error);
        CHECK(result.Directory == (folder.GetPath() / "MyGame").lexically_normal());
        REQUIRE(std::filesystem::is_directory(GetAssetsDirectory(result.Directory)));
        CHECK(std::filesystem::is_empty(GetAssetsDirectory(result.Directory)));
    }

    TEST_CASE("CreateProject creates the folders of the location that do not exist")
    {
        const TestHelpers::TemporaryFolder folder("quasar_create_location");
        const std::filesystem::path location = folder.GetPath() / "Quasar Projects" / "Games";

        const ProjectResult result = CreateProject(location, "MyGame");

        REQUIRE_MESSAGE(result.IsValid(), result.Error);
        CHECK(result.Directory == (location / "MyGame").lexically_normal());
        CHECK(std::filesystem::is_directory(GetAssetsDirectory(result.Directory)));
    }

    TEST_CASE("CreateProject names the project folder with the UTF-8 name as typed")
    {
        const TestHelpers::TemporaryFolder folder("quasar_create_utf8");
        const std::string name = "Proj\xC3\xA9t";

        const ProjectResult result = CreateProject(folder.GetPath(), name);

        REQUIRE_MESSAGE(result.IsValid(), result.Error);
        CHECK(PathToUtf8(result.Directory.filename()) == name);
    }

    TEST_CASE("CreateProject creates nothing when the name cannot be a folder name")
    {
        const TestHelpers::TemporaryFolder folder("quasar_create_bad_name");

        for (const char* name : {"", "..", "My/Game", "My\\Game", "Game."})
        {
            CAPTURE(name);
            CheckFailed(CreateProject(folder.GetPath(), name));
        }

        CHECK(std::filesystem::is_empty(folder.GetPath()));
    }

    TEST_CASE("CreateProject creates nothing when the location is relative")
    {
        const TestHelpers::TemporaryFolder folder("quasar_create_relative");
        const TestHelpers::ScopedCurrentDirectory currentDirectory;
        std::filesystem::current_path(folder.GetPath());

        CheckFailed(CreateProject("Projects", "MyGame"));
        CheckFailed(CreateProject(".", "MyGame"));

        CHECK(std::filesystem::is_empty(folder.GetPath()));
    }

    TEST_CASE("CreateProject leaves an existing project folder untouched")
    {
        const TestHelpers::TemporaryFolder folder("quasar_create_existing");
        REQUIRE(Quasar::WriteTextFile(folder.GetPath() / "MyGame" / "notes.txt", "content"));

        CheckFailed(CreateProject(folder.GetPath(), "MyGame"));

        CHECK_FALSE(std::filesystem::exists(folder.GetPath() / "MyGame" / "Assets"));
        CHECK(std::filesystem::exists(folder.GetPath() / "MyGame" / "notes.txt"));
    }

    TEST_CASE("CreateProject fails when a file is in the way of the location")
    {
        const TestHelpers::ScopedLog log;
        const TestHelpers::TemporaryFolder folder("quasar_create_over_file");
        REQUIRE(Quasar::WriteTextFile(folder.GetPath() / "file.txt", "content"));

        CheckFailed(CreateProject(folder.GetPath() / "file.txt", "MyGame"));
        CHECK(std::filesystem::is_regular_file(folder.GetPath() / "file.txt"));
    }

    TEST_CASE("OpenProject gives the normalized full path of a folder that has an Assets folder")
    {
        const TestHelpers::TemporaryFolder folder("quasar_open_project");
        REQUIRE(Quasar::CreateDirectories(folder.GetPath() / "MyGame" / "Assets"));

        const ProjectResult result = OpenProject(folder.GetPath() / "MyGame" / "Assets" / "..");

        REQUIRE_MESSAGE(result.IsValid(), result.Error);
        CHECK(result.Directory == (folder.GetPath() / "MyGame").lexically_normal());
    }

    TEST_CASE("OpenProject refuses a folder that is not a project")
    {
        const TestHelpers::TemporaryFolder folder("quasar_open_not_project");

        SUBCASE("the folder has no Assets folder")
        {
            REQUIRE(Quasar::CreateDirectories(folder.GetPath() / "MyGame"));
            CheckFailed(OpenProject(folder.GetPath() / "MyGame"));
        }
        SUBCASE("Assets is a file instead of a folder")
        {
            REQUIRE(Quasar::WriteTextFile(folder.GetPath() / "MyGame" / "Assets", "content"));
            CheckFailed(OpenProject(folder.GetPath() / "MyGame"));
        }
        SUBCASE("the folder does not exist")
        {
            CheckFailed(OpenProject(folder.GetPath() / "Missing"));
        }
        SUBCASE("no folder is given")
        {
            CheckFailed(OpenProject({}));
        }
    }
}
