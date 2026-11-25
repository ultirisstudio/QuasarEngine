#include "helpers/LogHelpers.h"
#include "helpers/TemporaryFiles.h"

#include <EditorSettings.h>

#include <quasar/io/File.h>
#include <quasar/io/Path.h>
#include <quasar/log/MemoryLogSink.h>

#include <doctest/doctest.h>

#include <cstddef>
#include <filesystem>
#include <string>
#include <vector>

using Quasar::Editor::EditorSettings;
using Quasar::Editor::MaxRecentProjects;

TEST_SUITE("EditorSettings")
{
    TEST_CASE("Settings without a file start empty and are not saved")
    {
        const TestHelpers::TemporaryFolder folder("quasar_settings_memory");
        EditorSettings settings;

        settings.RememberProject(folder.GetPath() / "Game");

        CHECK(settings.GetFile().empty());
        CHECK(settings.GetLastLocation() == folder.GetPath());
        CHECK(settings.GetRecentProjects() == std::vector{folder.GetPath() / "Game"});
        CHECK(std::filesystem::is_empty(folder.GetPath()));
    }

    TEST_CASE("A missing settings file gives empty settings without a warning")
    {
        const TestHelpers::ScopedLog log;
        const TestHelpers::TemporaryFolder folder("quasar_settings_missing");

        const EditorSettings settings(folder.GetPath() / "EditorSettings.yaml");

        CHECK(settings.GetLastLocation().empty());
        CHECK(settings.GetRecentProjects().empty());
        CHECK_FALSE(TestHelpers::HasLogAtLeast(Quasar::LogLevel::Warning));
        CHECK(std::filesystem::is_empty(folder.GetPath()));
    }

    TEST_CASE("The last location and the recent projects are kept between sessions")
    {
        const TestHelpers::TemporaryFolder folder("quasar_settings_saved");
        const std::filesystem::path file = folder.GetPath() / "Settings" / "EditorSettings.yaml";
        const std::filesystem::path projects = folder.GetPath() / "Proj\xC3\xA9ts";
        {
            EditorSettings settings(file);
            settings.RememberProject(projects / "First");
            settings.RememberProject(folder.GetPath() / "Second");
        }

        const EditorSettings settings(file);

        CHECK(settings.GetLastLocation() == folder.GetPath());
        CHECK(settings.GetRecentProjects() == std::vector{folder.GetPath() / "Second", projects / "First"});
    }

    TEST_CASE("Remembering a recent project again moves it first without a duplicate")
    {
        const TestHelpers::TemporaryFolder folder("quasar_settings_move");
        EditorSettings settings;
        settings.RememberProject(folder.GetPath() / "First");
        settings.RememberProject(folder.GetPath() / "Second");

        settings.RememberProject(folder.GetPath() / "Sub" / ".." / "First");

        CHECK(settings.GetRecentProjects() == std::vector{folder.GetPath() / "First", folder.GetPath() / "Second"});
    }

    TEST_CASE("Only the most recent projects are kept")
    {
        const TestHelpers::TemporaryFolder folder("quasar_settings_limit");
        const std::filesystem::path file = folder.GetPath() / "EditorSettings.yaml";
        EditorSettings settings(file);

        for (std::size_t index = 0; index <= MaxRecentProjects; ++index)
        {
            settings.RememberProject(folder.GetPath() / std::to_string(index));
        }

        REQUIRE(settings.GetRecentProjects().size() == MaxRecentProjects);
        CHECK(settings.GetRecentProjects().front() == folder.GetPath() / std::to_string(MaxRecentProjects));
        CHECK(settings.GetRecentProjects().back() == folder.GetPath() / "1");
        CHECK(EditorSettings(file).GetRecentProjects() == settings.GetRecentProjects());
    }

    TEST_CASE("Forgetting a project removes it from the saved recent projects")
    {
        const TestHelpers::TemporaryFolder folder("quasar_settings_forget");
        const std::filesystem::path file = folder.GetPath() / "EditorSettings.yaml";
        EditorSettings settings(file);
        settings.RememberProject(folder.GetPath() / "Kept");
        settings.RememberProject(folder.GetPath() / "Removed");

        settings.ForgetProject(folder.GetPath() / "Removed");
        settings.ForgetProject(folder.GetPath() / "Unknown");

        CHECK(settings.GetRecentProjects() == std::vector{folder.GetPath() / "Kept"});
        CHECK(EditorSettings(file).GetRecentProjects() == std::vector{folder.GetPath() / "Kept"});
    }

    TEST_CASE("A settings file that cannot be used gives empty settings")
    {
        const TestHelpers::ScopedLog log;
        const TestHelpers::TemporaryFolder folder("quasar_settings_invalid");
        const std::filesystem::path file = folder.GetPath() / "EditorSettings.yaml";
        bool warned = false;

        SUBCASE("not YAML")
        {
            REQUIRE(Quasar::WriteTextFile(file, "RecentProjects: [\n"));
            warned = true;
        }
        SUBCASE("a list instead of a map")
        {
            REQUIRE(Quasar::WriteTextFile(file, "- a\n- b\n"));
        }
        SUBCASE("values of the wrong type")
        {
            REQUIRE(Quasar::WriteTextFile(file, "LastLocation: [a]\nRecentProjects: text\n"));
        }

        const EditorSettings settings(file);

        CHECK(settings.GetLastLocation().empty());
        CHECK(settings.GetRecentProjects().empty());
        CHECK(TestHelpers::HasLogAtLeast(Quasar::LogLevel::Warning) == warned);
    }

    TEST_CASE("A settings file keeps only the full paths, once each")
    {
        const TestHelpers::TemporaryFolder folder("quasar_settings_paths");
        const std::filesystem::path file = folder.GetPath() / "EditorSettings.yaml";
        const std::string project = Quasar::PathToUtf8(folder.GetPath() / "Game");
        REQUIRE(Quasar::WriteTextFile(file, "LastLocation: relative\nRecentProjects:\n  - '" + project + "'\n  - relative\n  - [a]\n  - '" + project + "'\n"));

        const EditorSettings settings(file);

        CHECK(settings.GetLastLocation().empty());
        CHECK(settings.GetRecentProjects() == std::vector{folder.GetPath() / "Game"});
    }

    TEST_CASE("The language chosen is kept between sessions")
    {
        const TestHelpers::TemporaryFolder folder("quasar_settings_language");
        const std::filesystem::path file = folder.GetPath() / "EditorSettings.yaml";
        {
            EditorSettings settings(file);
            CHECK(settings.GetLanguage().empty());
            settings.SetLanguage("fr_FR");
        }

        EditorSettings settings(file);
        CHECK(settings.GetLanguage() == "fr_FR");
        settings.SetLanguage("");
        CHECK(EditorSettings(file).GetLanguage().empty());
    }
}
