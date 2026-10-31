#include <quasar/core/Log.h>
#include <quasar/core/PlatformDetection.h>
#include <quasar/io/UserConfigDirectory.h>

#include <doctest/doctest.h>

#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <optional>
#include <string>
#include <system_error>

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

namespace
{
    class ScopedEnvironmentVariable
    {
    public:
        ScopedEnvironmentVariable(const char* name, const std::optional<std::string>& value) : m_Name(name)
        {
            if (const char* previous = std::getenv(name); previous != nullptr)
            {
                m_Previous = previous;
            }
            Set(value);
        }

        ~ScopedEnvironmentVariable() { Set(m_Previous); }

        ScopedEnvironmentVariable(const ScopedEnvironmentVariable&) = delete;
        ScopedEnvironmentVariable& operator=(const ScopedEnvironmentVariable&) = delete;
        ScopedEnvironmentVariable(ScopedEnvironmentVariable&&) = delete;
        ScopedEnvironmentVariable& operator=(ScopedEnvironmentVariable&&) = delete;

    private:
        void Set(const std::optional<std::string>& value) const
        {
            if (value)
            {
                // NOLINTNEXTLINE(concurrency-mt-unsafe)
                setenv(m_Name.c_str(), value->c_str(), 1);
            }
            else
            {
                // NOLINTNEXTLINE(concurrency-mt-unsafe)
                unsetenv(m_Name.c_str());
            }
        }

        std::string m_Name;
        std::optional<std::string> m_Previous;
    };

    class TemporaryFolder
    {
    public:
        explicit TemporaryFolder(const char* name) : m_Path(std::filesystem::temp_directory_path() / name)
        {
            std::error_code error;
            std::filesystem::remove_all(m_Path, error);
            std::filesystem::create_directory(m_Path, error);
        }

        ~TemporaryFolder()
        {
            std::error_code error;
            std::filesystem::remove_all(m_Path, error);
        }

        TemporaryFolder(const TemporaryFolder&) = delete;
        TemporaryFolder& operator=(const TemporaryFolder&) = delete;
        TemporaryFolder(TemporaryFolder&&) = delete;
        TemporaryFolder& operator=(TemporaryFolder&&) = delete;

        const std::filesystem::path& GetPath() const { return m_Path; }

    private:
        std::filesystem::path m_Path;
    };
}

TEST_CASE("On Linux the folder is created in XDG_CONFIG_HOME when it does not exist yet")
{
    const TemporaryFolder root("quasar_config_xdg");
    const std::filesystem::path configHome = root.GetPath() / "missing" / "config";
    const ScopedEnvironmentVariable xdg("XDG_CONFIG_HOME", configHome.string());

    REQUIRE_FALSE(std::filesystem::exists(configHome));

    const std::optional<std::filesystem::path> directory = Quasar::GetUserConfigDirectory();

    REQUIRE(directory.has_value());
    CHECK(*directory == configHome / "QuasarEngine");
    CHECK(std::filesystem::is_directory(*directory));
}

TEST_CASE("On Linux the folder is in ~/.config without XDG_CONFIG_HOME")
{
    const TemporaryFolder home("quasar_config_home");
    const ScopedEnvironmentVariable xdg("XDG_CONFIG_HOME", std::nullopt);
    const ScopedEnvironmentVariable homeVariable("HOME", home.GetPath().string());

    const std::optional<std::filesystem::path> directory = Quasar::GetUserConfigDirectory();

    REQUIRE(directory.has_value());
    CHECK(*directory == home.GetPath() / ".config" / "QuasarEngine");
    CHECK(std::filesystem::is_directory(*directory));
}

TEST_CASE("On Linux a relative XDG_CONFIG_HOME is ignored")
{
    const TemporaryFolder home("quasar_config_relative");
    const ScopedEnvironmentVariable xdg("XDG_CONFIG_HOME", "relative/config");
    const ScopedEnvironmentVariable homeVariable("HOME", home.GetPath().string());

    const std::optional<std::filesystem::path> directory = Quasar::GetUserConfigDirectory();

    REQUIRE(directory.has_value());
    CHECK(*directory == home.GetPath() / ".config" / "QuasarEngine");
}

TEST_CASE("On Linux the home folder of the user is used without HOME")
{
    const ScopedEnvironmentVariable xdg("XDG_CONFIG_HOME", std::nullopt);
    const ScopedEnvironmentVariable homeVariable("HOME", std::nullopt);

    const std::optional<std::filesystem::path> directory = Quasar::GetUserConfigDirectory();

    REQUIRE(directory.has_value());
    CHECK(directory->is_absolute());
    CHECK(directory->parent_path().filename() == ".config");
}

TEST_CASE("On Linux no folder is returned when it cannot be created")
{
    Quasar::Log::Init();

    const TemporaryFolder root("quasar_config_file");
    const std::filesystem::path file = root.GetPath() / "file";
    std::ofstream(file) << "not a folder";
    const ScopedEnvironmentVariable xdg("XDG_CONFIG_HOME", file.string());

    CHECK_FALSE(Quasar::GetUserConfigDirectory().has_value());

    Quasar::Log::Shutdown();
}

#endif
