#include "LogHelpers.h"
#include "ScopedEnvironmentVariable.h"
#include "TemporaryFiles.h"

#include <quasar/core/PlatformDetection.h>
#include <quasar/io/UserConfigDirectory.h>

#include <doctest/doctest.h>

#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <optional>
#include <string>

TEST_CASE("The user config directory is an absolute QuasarEngine folder that exists")
{
    const std::optional<std::filesystem::path> directory = Quasar::GetUserConfigDirectory();

    REQUIRE(directory.has_value());
    CHECK(directory->is_absolute());
    CHECK(directory->filename() == "QuasarEngine");
    CHECK(std::filesystem::is_directory(*directory));
}

TEST_CASE("The user config directory is the same on every call")
{
    CHECK(Quasar::GetUserConfigDirectory() == Quasar::GetUserConfigDirectory());
}

#ifdef QUASAR_PLATFORM_LINUX

TEST_CASE("On Linux the folder is created in XDG_CONFIG_HOME when it does not exist yet")
{
    const TestHelpers::TemporaryFolder root("quasar_config_xdg");
    const std::filesystem::path configHome = root.GetPath() / "missing" / "config";
    const TestHelpers::ScopedEnvironmentVariable xdg("XDG_CONFIG_HOME", configHome.string());

    REQUIRE_FALSE(std::filesystem::exists(configHome));

    const std::optional<std::filesystem::path> directory = Quasar::GetUserConfigDirectory();

    REQUIRE(directory.has_value());
    CHECK(*directory == configHome / "QuasarEngine");
    CHECK(std::filesystem::is_directory(*directory));
}

TEST_CASE("On Linux the folder is in ~/.config without XDG_CONFIG_HOME")
{
    const TestHelpers::TemporaryFolder home("quasar_config_home");
    const TestHelpers::ScopedEnvironmentVariable xdg("XDG_CONFIG_HOME", std::nullopt);
    const TestHelpers::ScopedEnvironmentVariable homeVariable("HOME", home.GetPath().string());

    const std::optional<std::filesystem::path> directory = Quasar::GetUserConfigDirectory();

    REQUIRE(directory.has_value());
    CHECK(*directory == home.GetPath() / ".config" / "QuasarEngine");
    CHECK(std::filesystem::is_directory(*directory));
}

TEST_CASE("On Linux a relative XDG_CONFIG_HOME is ignored")
{
    const TestHelpers::TemporaryFolder home("quasar_config_relative");
    const TestHelpers::ScopedEnvironmentVariable xdg("XDG_CONFIG_HOME", "relative/config");
    const TestHelpers::ScopedEnvironmentVariable homeVariable("HOME", home.GetPath().string());

    const std::optional<std::filesystem::path> directory = Quasar::GetUserConfigDirectory();

    REQUIRE(directory.has_value());
    CHECK(*directory == home.GetPath() / ".config" / "QuasarEngine");
}

TEST_CASE("On Linux the home folder of the user is used without HOME")
{
    const TestHelpers::ScopedEnvironmentVariable xdg("XDG_CONFIG_HOME", std::nullopt);
    const TestHelpers::ScopedEnvironmentVariable homeVariable("HOME", std::nullopt);

    const std::optional<std::filesystem::path> directory = Quasar::GetUserConfigDirectory();

    REQUIRE(directory.has_value());
    CHECK(directory->is_absolute());
    CHECK(directory->parent_path().filename() == ".config");
}

TEST_CASE("On Linux no folder is returned when it cannot be created")
{
    const TestHelpers::ScopedLog log;

    const TestHelpers::TemporaryFolder root("quasar_config_file");
    const std::filesystem::path file = root.GetPath() / "file";
    std::ofstream(file) << "not a folder";
    const TestHelpers::ScopedEnvironmentVariable xdg("XDG_CONFIG_HOME", file.string());

    CHECK_FALSE(Quasar::GetUserConfigDirectory().has_value());
}

#endif
