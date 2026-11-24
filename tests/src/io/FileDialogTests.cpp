#include "helpers/FakeFileDialog.h"
#include "helpers/LogHelpers.h"

#include <quasar/core/Base.h>
#include <quasar/core/PlatformDetection.h>
#include <quasar/io/FileDialog.h>
#include <quasar/log/MemoryLogSink.h>

#include <doctest/doctest.h>

#include <filesystem>
#include <optional>
#include <string>
#include <thread>
#include <utility>

using TestHelpers::FolderDialogRequest;

namespace
{
    Quasar::Scope<Quasar::FileDialogBackend> CreateBackend(FolderDialogRequest& request)
    {
        return TestHelpers::CreateFakeFileDialogBackend(request);
    }
}

TEST_CASE("A new file dialog is closed and has no folder")
{
    FolderDialogRequest request;
    Quasar::FileDialog dialog(CreateBackend(request));

    CHECK_FALSE(dialog.IsOpen());
    CHECK_FALSE(dialog.TakeFolder().has_value());
    CHECK(request.Count == 0);
}

TEST_CASE("The folder chosen in the dialog is taken once, and the dialog is shown once at a time")
{
    const TestHelpers::ScopedLog log;
    FolderDialogRequest request;
    Quasar::FileDialog dialog(CreateBackend(request));
    const std::filesystem::path location = std::filesystem::temp_directory_path();

    dialog.OpenFolder(nullptr, location);
    CHECK(dialog.IsOpen());
    CHECK(request.DefaultLocation == location);
    dialog.OpenFolder(nullptr, {});
    CHECK(request.Count == 1);
    CHECK_FALSE(dialog.TakeFolder().has_value());

    request.Callback({.Folder = location / "Chosen"});
    CHECK_FALSE(dialog.IsOpen());
    CHECK(dialog.TakeFolder() == location / "Chosen");
    CHECK_FALSE(dialog.TakeFolder().has_value());

    dialog.OpenFolder(nullptr, {});
    CHECK(request.Count == 2);
    request.Callback({.Folder = location / "Forgotten"});
    dialog.OpenFolder(nullptr, {});
    CHECK(request.Count == 3);
    CHECK_FALSE(dialog.TakeFolder().has_value());
    CHECK_FALSE(TestHelpers::HasLogAtLeast(Quasar::LogLevel::Warning));
}

TEST_CASE("A canceled folder dialog gives no folder, and one that cannot be shown logs why")
{
    const TestHelpers::ScopedLog log;
    FolderDialogRequest request;
    Quasar::FileDialog dialog(CreateBackend(request));

    dialog.OpenFolder(nullptr, {});
    request.Callback({});
    CHECK_FALSE(dialog.IsOpen());
    CHECK_FALSE(dialog.TakeFolder().has_value());
    CHECK_FALSE(TestHelpers::HasLogAtLeast(Quasar::LogLevel::Warning));

    dialog.OpenFolder(nullptr, {});
    request.Callback({.Error = "no dialog on this system"});
    CHECK_FALSE(dialog.IsOpen());
    CHECK_FALSE(dialog.TakeFolder().has_value());
    CHECK(TestHelpers::CountErrors("no dialog on this system") == 1);
}

TEST_CASE("A folder dialog takes an answer given on another thread, even after it is destroyed")
{
    const TestHelpers::ScopedLog log;
    FolderDialogRequest request;
    {
        Quasar::FileDialog dialog(CreateBackend(request));
        dialog.OpenFolder(nullptr, {});
        std::thread answer([&request] { request.Callback({.Folder = std::filesystem::path("Chosen")}); });
        answer.join();
        CHECK(dialog.TakeFolder() == std::filesystem::path("Chosen"));

        dialog.OpenFolder(nullptr, {});
    }
    request.Callback({.Folder = std::filesystem::path("Late")});
    CHECK_FALSE(TestHelpers::HasLogAtLeast(Quasar::LogLevel::Warning));
}

#ifdef QUASAR_PLATFORM_LINUX

#include <SDL3/SDL_hints.h>

#include <chrono>

TEST_CASE("The dialog of the system that cannot be shown logs an error and closes")
{
    const TestHelpers::ScopedLog log;
    SDL_SetHint(SDL_HINT_FILE_DIALOG_DRIVER, "none");
    Quasar::FileDialog dialog;

    dialog.OpenFolder(nullptr, {});

    const auto deadline = std::chrono::steady_clock::now() + std::chrono::seconds(10);
    while (dialog.IsOpen() && std::chrono::steady_clock::now() < deadline)
    {
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
    }
    SDL_ResetHint(SDL_HINT_FILE_DIALOG_DRIVER);
    CHECK_FALSE(dialog.IsOpen());
    CHECK_FALSE(dialog.TakeFolder().has_value());
    CHECK(TestHelpers::CountErrors("Cannot show the folder dialog") == 1);
}

#endif
