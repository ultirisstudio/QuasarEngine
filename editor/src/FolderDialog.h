#pragma once

#include <filesystem>
#include <memory>
#include <optional>
#include <string>

struct SDL_Window;

namespace Quasar::Editor
{
    struct FolderDialogState;

    class FolderDialog
    {
    public:
        FolderDialog();
        ~FolderDialog() = default;

        FolderDialog(const FolderDialog&) = delete;
        FolderDialog& operator=(const FolderDialog&) = delete;
        FolderDialog(FolderDialog&&) = delete;
        FolderDialog& operator=(FolderDialog&&) = delete;

        void Open(SDL_Window* window, const std::filesystem::path& defaultLocation);

        [[nodiscard]] bool IsOpen() const;

        [[nodiscard]] std::optional<std::filesystem::path> TakeFolder();

        [[nodiscard]] std::string TakeError();

    private:
        std::shared_ptr<FolderDialogState> m_State;
    };
}
