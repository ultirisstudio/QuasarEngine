#include <quasar/platform/FileDialog.h>

#include <quasar/core/PlatformDetection.h>

#include <doctest/doctest.h>

TEST_CASE("A new file dialog is closed and has no folder")
{
    Quasar::FileDialog dialog;

    CHECK_FALSE(dialog.IsOpen());
    CHECK_FALSE(dialog.TakeFolder().has_value());
}

#ifdef QUASAR_PLATFORM_LINUX

#include "helpers/LogHelpers.h"
#include "helpers/ScopedEnvironmentVariable.h"
#include "helpers/TemporaryFiles.h"

#include <quasar/io/File.h>
#include <quasar/io/Path.h>

#include <SDL3/SDL_hints.h>

#include <chrono>
#include <cstdlib>
#include <filesystem>
#include <optional>
#include <string>
#include <string_view>
#include <thread>

namespace
{
    class ScopedFileDialogDriver
    {
    public:
        explicit ScopedFileDialogDriver(const char* driver) { SDL_SetHint(SDL_HINT_FILE_DIALOG_DRIVER, driver); }
        ~ScopedFileDialogDriver() { SDL_ResetHint(SDL_HINT_FILE_DIALOG_DRIVER); }

        ScopedFileDialogDriver(const ScopedFileDialogDriver&) = delete;
        ScopedFileDialogDriver& operator=(const ScopedFileDialogDriver&) = delete;
        ScopedFileDialogDriver(ScopedFileDialogDriver&&) = delete;
        ScopedFileDialogDriver& operator=(ScopedFileDialogDriver&&) = delete;
    };

    class FakeZenity
    {
    public:
        explicit FakeZenity(std::string_view answer) : m_SearchPath("PATH", m_Folder.GetPath().string() + ":" + GetSearchPath())
        {
            CreateScript(m_Folder.GetPath(), answer);
            SDL_SetHint(SDL_HINT_FILE_DIALOG_DRIVER, "zenity");
        }

        ~FakeZenity() { SDL_ResetHint(SDL_HINT_FILE_DIALOG_DRIVER); }

        FakeZenity(const FakeZenity&) = delete;
        FakeZenity& operator=(const FakeZenity&) = delete;
        FakeZenity(FakeZenity&&) = delete;
        FakeZenity& operator=(FakeZenity&&) = delete;

    private:
        static std::string GetSearchPath()
        {
            // NOLINTNEXTLINE(concurrency-mt-unsafe): the tests that change the environment start no thread before
            const char* path = std::getenv("PATH");
            return path != nullptr ? path : "";
        }

        static void CreateScript(const std::filesystem::path& folder, std::string_view answer)
        {
            const std::filesystem::path script = folder / "zenity";
            const std::string text = "#!/bin/sh\nif [ \"$1\" = \"--version\" ]; then echo 4.0.1; exit 0; fi\n" + std::string(answer) + "\n";
            REQUIRE(Quasar::WriteTextFile(script, text));
            std::filesystem::permissions(script, std::filesystem::perms::owner_all);
        }

        TestHelpers::TemporaryFolder m_Folder{"quasar_fake_zenity"};
        TestHelpers::ScopedEnvironmentVariable m_SearchPath;
    };

    bool WaitUntilClosed(const Quasar::FileDialog& dialog)
    {
        const auto deadline = std::chrono::steady_clock::now() + std::chrono::seconds(10);
        while (dialog.IsOpen())
        {
            if (std::chrono::steady_clock::now() > deadline)
            {
                return false;
            }
            std::this_thread::sleep_for(std::chrono::milliseconds(10));
        }
        return true;
    }
}

TEST_CASE("A folder dialog that cannot be shown logs an error and closes")
{
    const TestHelpers::ScopedLog log;
    const ScopedFileDialogDriver driver("none");
    Quasar::FileDialog dialog;

    dialog.OpenFolder(nullptr, {});

    REQUIRE(WaitUntilClosed(dialog));
    CHECK_FALSE(dialog.TakeFolder().has_value());
    CHECK(TestHelpers::HasLogAtLeast(Quasar::LogLevel::Error));
}

TEST_CASE("The folder chosen in the dialog is taken once")
{
    const TestHelpers::ScopedLog log;
    const std::string folder = "/tmp/Quasar Proj\xC3\xA9ts";
    const FakeZenity zenity("printf '%s\\n' '" + folder + "'");
    Quasar::FileDialog dialog;

    dialog.OpenFolder(nullptr, {});

    REQUIRE(WaitUntilClosed(dialog));
    const std::optional<std::filesystem::path> chosen = dialog.TakeFolder();
    REQUIRE(chosen.has_value());
    CHECK(*chosen == Quasar::PathFromUtf8(folder));
    CHECK_FALSE(dialog.TakeFolder().has_value());
    CHECK_FALSE(TestHelpers::HasLogAtLeast(Quasar::LogLevel::Error));
}

TEST_CASE("A canceled folder dialog gives no folder")
{
    const TestHelpers::ScopedLog log;
    const FakeZenity zenity("exit 1");
    Quasar::FileDialog dialog;

    dialog.OpenFolder(nullptr, std::filesystem::temp_directory_path());

    REQUIRE(WaitUntilClosed(dialog));
    CHECK_FALSE(dialog.TakeFolder().has_value());
    CHECK_FALSE(TestHelpers::HasLogAtLeast(Quasar::LogLevel::Error));
}

#endif
