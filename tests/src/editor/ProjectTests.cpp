#include <Project.h>

#include <doctest/doctest.h>

#include <filesystem>
#include <fstream>
#include <random>
#include <string>
#include <system_error>

namespace Quasar::Editor
{
    namespace
    {
        /// A folder of its own for each test, removed with everything inside when the test ends.
        class ScratchFolder
        {
        public:
            ScratchFolder() : m_Root(std::filesystem::temp_directory_path() / ("QuasarProject-" + std::to_string(std::random_device{}())))
            {
                std::filesystem::create_directories(m_Root);
            }

            ~ScratchFolder()
            {
                std::error_code error;
                std::filesystem::remove_all(m_Root, error);
            }

            ScratchFolder(const ScratchFolder&) = delete;
            ScratchFolder& operator=(const ScratchFolder&) = delete;
            ScratchFolder(ScratchFolder&&) = delete;
            ScratchFolder& operator=(ScratchFolder&&) = delete;

            [[nodiscard]] const std::filesystem::path& Root() const { return m_Root; }

            void WriteFile(const std::filesystem::path& path) const
            {
                std::ofstream(m_Root / path) << "content";
            }

        private:
            std::filesystem::path m_Root;
        };

        void CheckFailed(const ProjectResult& result)
        {
            CHECK_FALSE(result.IsValid());
            CHECK_FALSE(result.Error.empty());
            CHECK(result.Directory.empty());
        }
    }

    TEST_SUITE("Project")
    {
        TEST_CASE("PathToUtf8 gives back the UTF-8 text given to PathFromUtf8")
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

        TEST_CASE_FIXTURE(ScratchFolder, "ResolveProjectDirectory gives the normalized full path of the project folder")
        {
            const std::filesystem::path expected = (Root() / "MyGame").lexically_normal();

            SUBCASE("from a plain location")
            {
                CHECK(ResolveProjectDirectory(Root(), "MyGame") == expected);
            }
            SUBCASE("from a location with dot segments")
            {
                CHECK(ResolveProjectDirectory(Root() / "Sub" / ".." / ".", "MyGame") == expected);
            }
        }

        TEST_CASE("ResolveProjectDirectory refuses a location that is not a full path")
        {
            CHECK(ResolveProjectDirectory("", "MyGame").empty());
            CHECK(ResolveProjectDirectory("Projects", "MyGame").empty());
            CHECK(ResolveProjectDirectory(std::filesystem::path(".") / "Projects", "MyGame").empty());
        }

        TEST_CASE_FIXTURE(ScratchFolder, "ResolveProjectDirectory refuses a name that cannot be a folder name")
        {
            for (const char* name : {"", ".", "..", "My/Game", "My\\Game", "Game.", "Game ", "Ga:me", "Ga*me", "Tab\tName"})
            {
                CAPTURE(name);
                CHECK(ResolveProjectDirectory(Root(), name).empty());
            }
        }

        TEST_CASE_FIXTURE(ScratchFolder, "CreateProject creates an empty Assets folder inside the new project folder")
        {
            const ProjectResult result = CreateProject(Root(), "MyGame");

            REQUIRE_MESSAGE(result.IsValid(), result.Error);
            CHECK(result.Directory == (Root() / "MyGame").lexically_normal());
            REQUIRE(std::filesystem::is_directory(GetAssetsDirectory(result.Directory)));
            CHECK(std::filesystem::is_empty(GetAssetsDirectory(result.Directory)));
        }

        TEST_CASE_FIXTURE(ScratchFolder, "CreateProject names the project folder with the UTF-8 name as typed")
        {
            const std::string name = "Proj\xC3\xA9t";

            const ProjectResult result = CreateProject(Root(), name);

            REQUIRE_MESSAGE(result.IsValid(), result.Error);
            CHECK(PathToUtf8(result.Directory.filename()) == name);
        }

        TEST_CASE_FIXTURE(ScratchFolder, "CreateProject creates nothing when the name cannot be a folder name")
        {
            for (const char* name : {"", "..", "My/Game", "My\\Game", "Game."})
            {
                CAPTURE(name);
                CheckFailed(CreateProject(Root(), name));
            }

            CHECK(std::filesystem::is_empty(Root()));
        }

        TEST_CASE("CreateProject creates nothing when the location is relative")
        {
            const std::string name = "QuasarRelativeProject-" + std::to_string(std::random_device{}());

            CheckFailed(CreateProject(".", name));
            CHECK_FALSE(std::filesystem::exists(std::filesystem::current_path() / name));
        }

        TEST_CASE_FIXTURE(ScratchFolder, "CreateProject leaves an existing folder untouched")
        {
            std::filesystem::create_directory(Root() / "MyGame");
            WriteFile(std::filesystem::path("MyGame") / "notes.txt");

            CheckFailed(CreateProject(Root(), "MyGame"));

            CHECK_FALSE(std::filesystem::exists(Root() / "MyGame" / "Assets"));
            CHECK(std::filesystem::exists(Root() / "MyGame" / "notes.txt"));
        }

        TEST_CASE_FIXTURE(ScratchFolder, "CreateProject fails when the location is not an existing folder")
        {
            SUBCASE("the location does not exist")
            {
                CheckFailed(CreateProject(Root() / "Missing", "MyGame"));
                CHECK_FALSE(std::filesystem::exists(Root() / "Missing"));
            }
            SUBCASE("the location is a file")
            {
                WriteFile("file.txt");
                CheckFailed(CreateProject(Root() / "file.txt", "MyGame"));
            }
        }

        TEST_CASE_FIXTURE(ScratchFolder, "OpenProject gives the normalized full path of a folder that has an Assets folder")
        {
            std::filesystem::create_directories(Root() / "MyGame" / "Assets");

            const ProjectResult result = OpenProject(Root() / "MyGame" / "Assets" / "..");

            REQUIRE_MESSAGE(result.IsValid(), result.Error);
            CHECK(result.Directory == (Root() / "MyGame").lexically_normal());
        }

        TEST_CASE_FIXTURE(ScratchFolder, "OpenProject refuses a folder that is not a project")
        {
            SUBCASE("the folder has no Assets folder")
            {
                std::filesystem::create_directory(Root() / "MyGame");
                CheckFailed(OpenProject(Root() / "MyGame"));
            }
            SUBCASE("Assets is a file instead of a folder")
            {
                std::filesystem::create_directory(Root() / "MyGame");
                WriteFile(std::filesystem::path("MyGame") / "Assets");
                CheckFailed(OpenProject(Root() / "MyGame"));
            }
            SUBCASE("the folder does not exist")
            {
                CheckFailed(OpenProject(Root() / "Missing"));
            }
            SUBCASE("no folder is given")
            {
                CheckFailed(OpenProject({}));
            }
        }
    }
}
