#include <Project.h>

#include "helpers/LogHelpers.h"
#include "helpers/TemporaryFiles.h"

#include <quasar/io/File.h>
#include <quasar/io/Path.h>

#include <doctest/doctest.h>

#include <filesystem>
#include <string>
#include <string_view>
#include <system_error>

using Quasar::PathFromUtf8;
using Quasar::PathToUtf8;
using Quasar::Editor::AssetsDirectoryName;
using Quasar::Editor::ChooseDefaultProjectLocation;
using Quasar::Editor::CreateProject;
using Quasar::Editor::DefaultProjectsDirectoryName;
using Quasar::Editor::GetAssetsDirectory;
using Quasar::Editor::GetDefaultProjectLocation;
using Quasar::Editor::GetProjectFile;
using Quasar::Editor::OpenProject;
using Quasar::Editor::ProjectFileExtension;
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

    bool Contains(std::string_view text, std::string_view part)
    {
        return text.find(part) != std::string_view::npos;
    }

    bool MentionsPath(const ProjectResult& result, const std::filesystem::path& path)
    {
        return Contains(result.Error, PathToUtf8(path.lexically_normal()));
    }

    std::filesystem::path ProjectFileOf(const std::filesystem::path& directory, std::string_view name)
    {
        return directory / PathFromUtf8(std::string(name) + std::string(ProjectFileExtension));
    }

    void MakeProjectFolder(const std::filesystem::path& directory)
    {
        REQUIRE(Quasar::CreateDirectories(GetAssetsDirectory(directory)));
        REQUIRE(Quasar::WriteTextFile(GetProjectFile(directory), ""));
    }

    bool MakeLinkToItself(const std::filesystem::path& link)
    {
        std::error_code error;
        std::filesystem::create_directory_symlink(link.filename(), link, error);
        if (error)
        {
            MESSAGE("This computer cannot create symbolic links: ", error.message());
            return false;
        }
        return true;
    }

    std::string GetStatusError(const std::filesystem::path& path)
    {
        std::error_code error;
        static_cast<void>(std::filesystem::status(path, error));
        return error.message();
    }
}

TEST_SUITE("Project")
{
    TEST_CASE("GetAssetsDirectory points to the Assets folder inside the project folder")
    {
        const std::filesystem::path project = std::filesystem::temp_directory_path() / "MyGame";

        const std::filesystem::path assets = GetAssetsDirectory(project);

        CHECK(assets.filename() == AssetsDirectoryName);
        CHECK(assets.parent_path() == project);
    }

    TEST_CASE("GetProjectFile is the project file named after the project folder")
    {
        const std::filesystem::path project = std::filesystem::temp_directory_path() / "MyGame";

        CHECK(GetProjectFile(project) == ProjectFileOf(project, "MyGame"));
        CHECK(GetProjectFile(project / "") == ProjectFileOf(project, "MyGame"));
    }

    TEST_CASE("GetDefaultProjectLocation is a full path that ends with the default projects folder")
    {
        const TestHelpers::ScopedLog log;

        const std::filesystem::path location = GetDefaultProjectLocation();

        REQUIRE_FALSE(location.empty());
        CHECK(location.is_absolute());
        CHECK(location.filename() == DefaultProjectsDirectoryName);
    }

    TEST_CASE("ChooseDefaultProjectLocation puts the projects in the documents folder")
    {
        const std::filesystem::path home = std::filesystem::temp_directory_path() / "User";

        CHECK(ChooseDefaultProjectLocation(home / "Documents" / "", home) == (home / "Documents" / DefaultProjectsDirectoryName).lexically_normal());
    }

    TEST_CASE("ChooseDefaultProjectLocation avoids a documents folder synced by OneDrive")
    {
        const std::filesystem::path home = std::filesystem::temp_directory_path() / "User";
        const std::filesystem::path expected = (home / "Documents" / DefaultProjectsDirectoryName).lexically_normal();

        CHECK(ChooseDefaultProjectLocation(home / "OneDrive" / "Documents", home) == expected);
        CHECK(ChooseDefaultProjectLocation(home / "OneDrive - Ultiris Studio" / "Documents", home) == expected);
    }

    TEST_CASE("ChooseDefaultProjectLocation keeps folders whose name only looks like OneDrive")
    {
        const std::filesystem::path home = std::filesystem::temp_directory_path() / "User";

        for (const char* folder : {"MyOneDrive", "OneDriveFan"})
        {
            CAPTURE(folder);
            const std::filesystem::path documents = home / folder / "Documents";
            CHECK(ChooseDefaultProjectLocation(documents, home) == (documents / DefaultProjectsDirectoryName).lexically_normal());
        }
    }

    TEST_CASE("ChooseDefaultProjectLocation uses the folders that it is given when it has no better choice")
    {
        const std::filesystem::path home = std::filesystem::temp_directory_path() / "User";
        const std::filesystem::path oneDrive = home / "OneDrive" / "Documents";

        CHECK(ChooseDefaultProjectLocation({}, home) == (home / DefaultProjectsDirectoryName).lexically_normal());
        CHECK(ChooseDefaultProjectLocation(oneDrive, {}) == (oneDrive / DefaultProjectsDirectoryName).lexically_normal());
        CHECK(ChooseDefaultProjectLocation({}, {}).empty());
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

    TEST_CASE("ResolveProjectDirectory refuses the names that Windows reserves for devices")
    {
        const TestHelpers::TemporaryFolder folder("quasar_resolve_reserved");

        for (const char* name : {"CON", "nul", "Aux", "PRN", "COM1", "lpt9", "CON.txt"})
        {
            CAPTURE(name);
            CHECK(ResolveProjectDirectory(folder.GetPath(), name).empty());
        }
        CHECK_FALSE(ResolveProjectDirectory(folder.GetPath(), "Console").empty());
        CHECK_FALSE(ResolveProjectDirectory(folder.GetPath(), "COM10").empty());
    }

    TEST_CASE("CreateProject creates the project file and an empty Assets folder")
    {
        const TestHelpers::TemporaryFolder folder("quasar_create_project");

        const ProjectResult result = CreateProject(folder.GetPath(), "MyGame");

        REQUIRE_MESSAGE(result.IsValid(), result.Error);
        CHECK(result.Directory == (folder.GetPath() / "MyGame").lexically_normal());
        CHECK(std::filesystem::is_regular_file(ProjectFileOf(result.Directory, "MyGame")));
        CHECK(std::filesystem::is_empty(ProjectFileOf(result.Directory, "MyGame")));
        REQUIRE(std::filesystem::is_directory(GetAssetsDirectory(result.Directory)));
        CHECK(std::filesystem::is_empty(GetAssetsDirectory(result.Directory)));
    }

    TEST_CASE("CreateProject creates the folders of the location that do not exist")
    {
        const TestHelpers::TemporaryFolder folder("quasar_create_location");
        const std::filesystem::path location = folder.GetPath() / "Projects" / "Games";

        const ProjectResult result = CreateProject(location, "MyGame");

        REQUIRE_MESSAGE(result.IsValid(), result.Error);
        CHECK(result.Directory == (location / "MyGame").lexically_normal());
        CHECK(std::filesystem::is_directory(GetAssetsDirectory(result.Directory)));
    }

    TEST_CASE("CreateProject names the project folder and file with the UTF-8 name as typed")
    {
        const TestHelpers::TemporaryFolder folder("quasar_create_utf8");
        const std::string name = "Proj\xC3\xA9t";

        const ProjectResult result = CreateProject(folder.GetPath(), name);

        REQUIRE_MESSAGE(result.IsValid(), result.Error);
        CHECK(PathToUtf8(result.Directory.filename()) == name);
        CHECK(std::filesystem::is_regular_file(ProjectFileOf(result.Directory, name)));
    }

    TEST_CASE("A created project can be opened")
    {
        const TestHelpers::TemporaryFolder folder("quasar_create_then_open");

        const ProjectResult created = CreateProject(folder.GetPath(), "MyGame");
        REQUIRE_MESSAGE(created.IsValid(), created.Error);

        const ProjectResult opened = OpenProject(created.Directory);

        REQUIRE_MESSAGE(opened.IsValid(), opened.Error);
        CHECK(opened.Directory == created.Directory);
    }

    TEST_CASE("CreateProject creates nothing when the name cannot be a folder name")
    {
        const TestHelpers::TemporaryFolder folder("quasar_create_bad_name");

        for (const char* name : {"", "..", "My/Game", "My\\Game", "Game.", "NUL"})
        {
            CAPTURE(name);
            CheckFailed(CreateProject(folder.GetPath(), name));
        }

        CHECK(std::filesystem::is_empty(folder.GetPath()));
    }

    TEST_CASE("CreateProject creates nothing when the location is empty or relative")
    {
        const TestHelpers::TemporaryFolder folder("quasar_create_relative");
        const TestHelpers::ScopedCurrentDirectory currentDirectory;
        std::filesystem::current_path(folder.GetPath());

        CheckFailed(CreateProject({}, "MyGame"));
        CheckFailed(CreateProject("Projects", "MyGame"));
        CheckFailed(CreateProject(".", "MyGame"));

        CHECK(std::filesystem::is_empty(folder.GetPath()));
    }

    TEST_CASE("CreateProject leaves an existing folder untouched")
    {
        const TestHelpers::TemporaryFolder folder("quasar_create_existing");
        REQUIRE(Quasar::WriteTextFile(folder.GetPath() / "MyGame" / "notes.txt", "content"));

        const ProjectResult result = CreateProject(folder.GetPath(), "MyGame");

        CheckFailed(result);
        CHECK(MentionsPath(result, folder.GetPath() / "MyGame"));
        CHECK_FALSE(std::filesystem::exists(GetAssetsDirectory(folder.GetPath() / "MyGame")));
        CHECK_FALSE(std::filesystem::exists(ProjectFileOf(folder.GetPath() / "MyGame", "MyGame")));
        CHECK(std::filesystem::exists(folder.GetPath() / "MyGame" / "notes.txt"));
    }

    TEST_CASE("CreateProject gives the reason when a file is in the way of the location")
    {
        const TestHelpers::TemporaryFolder folder("quasar_create_over_file");
        REQUIRE(Quasar::WriteTextFile(folder.GetPath() / "file.txt", "content"));
        const std::filesystem::path assets = GetAssetsDirectory(folder.GetPath() / "file.txt" / "MyGame");
        std::error_code expected;
        std::filesystem::create_directories(assets, expected);
        REQUIRE(expected);

        const ProjectResult result = CreateProject(folder.GetPath() / "file.txt", "MyGame");

        CheckFailed(result);
        CHECK(MentionsPath(result, assets));
        CHECK(Contains(result.Error, expected.message()));
        CHECK(std::filesystem::is_regular_file(folder.GetPath() / "file.txt"));
    }

    TEST_CASE("CreateProject gives the reason when the name is too long for the file system")
    {
        const TestHelpers::TemporaryFolder folder("quasar_create_long_name");

        const ProjectResult result = CreateProject(folder.GetPath(), std::string(256, 'a'));

        CheckFailed(result);
        CHECK(MentionsPath(result, folder.GetPath()));
        CHECK(std::filesystem::is_empty(folder.GetPath()));
    }

    TEST_CASE("CreateProject removes the project folder when its project file cannot be written")
    {
        const TestHelpers::TemporaryFolder folder("quasar_create_long_file");
        const std::string name(250, 'b');

        const ProjectResult result = CreateProject(folder.GetPath(), name);

        CheckFailed(result);
        CHECK(Contains(result.Error, name));
        CHECK(std::filesystem::is_empty(folder.GetPath()));
    }

    TEST_CASE("CreateProject gives the reason when the location cannot be read")
    {
        const TestHelpers::TemporaryFolder folder("quasar_create_loop");
        const std::filesystem::path loop = folder.GetPath() / "Loop";
        if (!MakeLinkToItself(loop))
        {
            return;
        }

        const ProjectResult result = CreateProject(loop, "MyGame");

        CheckFailed(result);
        CHECK(MentionsPath(result, loop / "MyGame"));
    }

    TEST_CASE("OpenProject gives the normalized full path of a project folder")
    {
        const TestHelpers::TemporaryFolder folder("quasar_open_project");
        MakeProjectFolder(folder.GetPath() / "MyGame");

        const ProjectResult result = OpenProject(folder.GetPath() / "MyGame" / AssetsDirectoryName / "..");

        REQUIRE_MESSAGE(result.IsValid(), result.Error);
        CHECK(result.Directory == (folder.GetPath() / "MyGame").lexically_normal());
    }

    TEST_CASE("OpenProject finds a project from a path relative to the current folder")
    {
        const TestHelpers::TemporaryFolder folder("quasar_open_relative");
        MakeProjectFolder(folder.GetPath() / "MyGame");
        const TestHelpers::ScopedCurrentDirectory currentDirectory;
        std::filesystem::current_path(folder.GetPath());

        const ProjectResult result = OpenProject("MyGame");

        REQUIRE_MESSAGE(result.IsValid(), result.Error);
        CHECK(result.Directory == (std::filesystem::current_path() / "MyGame").lexically_normal());
    }

    TEST_CASE("OpenProject accepts a project file that has another name than the folder")
    {
        const TestHelpers::TemporaryFolder folder("quasar_open_renamed");
        REQUIRE(Quasar::CreateDirectories(GetAssetsDirectory(folder.GetPath() / "Renamed")));
        REQUIRE(Quasar::WriteTextFile(ProjectFileOf(folder.GetPath() / "Renamed", "MyGame"), ""));

        const ProjectResult result = OpenProject(folder.GetPath() / "Renamed");

        REQUIRE_MESSAGE(result.IsValid(), result.Error);
    }

    TEST_CASE("OpenProject refuses a folder that is not a project")
    {
        const TestHelpers::TemporaryFolder folder("quasar_open_not_project");
        const std::filesystem::path project = folder.GetPath() / "MyGame";

        SUBCASE("the folder has an Assets folder but no project file")
        {
            REQUIRE(Quasar::CreateDirectories(GetAssetsDirectory(project)));
        }
        SUBCASE("the project file is a folder")
        {
            REQUIRE(Quasar::CreateDirectories(GetAssetsDirectory(project)));
            REQUIRE(Quasar::CreateDirectories(ProjectFileOf(project, "MyGame")));
        }
        SUBCASE("the project has no Assets folder")
        {
            REQUIRE(Quasar::WriteTextFile(ProjectFileOf(project, "MyGame"), ""));
        }
        SUBCASE("Assets is a file instead of a folder")
        {
            REQUIRE(Quasar::WriteTextFile(ProjectFileOf(project, "MyGame"), ""));
            REQUIRE(Quasar::WriteTextFile(GetAssetsDirectory(project), "content"));
        }
        SUBCASE("the path is a file")
        {
            REQUIRE(Quasar::WriteTextFile(project, "content"));
        }
        SUBCASE("the folder does not exist") {}

        const ProjectResult result = OpenProject(project);

        CheckFailed(result);
        CHECK(MentionsPath(result, project));
    }

    TEST_CASE("OpenProject refuses an empty path")
    {
        CheckFailed(OpenProject({}));
    }

    TEST_CASE("OpenProject gives the reason when the folder cannot be read")
    {
        const TestHelpers::TemporaryFolder folder("quasar_open_loop");
        const std::filesystem::path loop = folder.GetPath() / "Loop";
        if (!MakeLinkToItself(loop))
        {
            return;
        }

        const ProjectResult result = OpenProject(loop);

        CheckFailed(result);
        CHECK(MentionsPath(result, loop));
        CHECK(Contains(result.Error, GetStatusError(loop)));
    }

    TEST_CASE("OpenProject gives the reason when a project file cannot be read")
    {
        const TestHelpers::TemporaryFolder folder("quasar_open_file_loop");
        const std::filesystem::path project = folder.GetPath() / "MyGame";
        REQUIRE(Quasar::CreateDirectories(GetAssetsDirectory(project)));
        const std::filesystem::path projectFile = ProjectFileOf(project, "MyGame");
        if (!MakeLinkToItself(projectFile))
        {
            return;
        }

        const ProjectResult result = OpenProject(project);

        CheckFailed(result);
        CHECK(MentionsPath(result, project));
        CHECK(Contains(result.Error, GetStatusError(projectFile)));
    }

    TEST_CASE("OpenProject gives the reason when the Assets folder cannot be read")
    {
        const TestHelpers::TemporaryFolder folder("quasar_open_assets_loop");
        const std::filesystem::path project = folder.GetPath() / "MyGame";
        REQUIRE(Quasar::WriteTextFile(ProjectFileOf(project, "MyGame"), ""));
        if (!MakeLinkToItself(GetAssetsDirectory(project)))
        {
            return;
        }

        const ProjectResult result = OpenProject(project);

        CheckFailed(result);
        CHECK(MentionsPath(result, GetAssetsDirectory(project)));
        CHECK(Contains(result.Error, GetStatusError(GetAssetsDirectory(project))));
    }
}
