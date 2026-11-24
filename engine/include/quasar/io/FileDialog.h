#pragma once

/// \file
/// \brief The native dialog of the system to choose a folder.
/// \author Fabien Wahbeh
/// \date 2026-11-23

#include <quasar/core/Base.h>

#include <filesystem>
#include <functional>
#include <memory>
#include <optional>
#include <string>

namespace Quasar
{
    class Window;

    /// @brief What a folder dialog gives when it closes.
    struct FileDialogResult
    {
        std::optional<std::filesystem::path> Folder{}; ///< The chosen folder, or no value if the dialog was canceled or could not be shown.
        std::string Error{};                           ///< Why the dialog could not be shown, or empty.
    };

    /// @brief Where the dialogs are shown. The engine shows the dialogs of the system with SDL, and the tests use their
    /// own backend to answer instead of a user.
    class FileDialogBackend
    {
    public:
        /// @brief Receives the result of a dialog, once, possibly from another thread.
        using Callback = std::function<void(FileDialogResult)>;

        FileDialogBackend() = default;
        virtual ~FileDialogBackend() = default;

        FileDialogBackend(const FileDialogBackend&) = delete;
        FileDialogBackend& operator=(const FileDialogBackend&) = delete;
        FileDialogBackend(FileDialogBackend&&) = delete;
        FileDialogBackend& operator=(FileDialogBackend&&) = delete;

        /// @brief Shows a dialog to choose one folder, and returns without waiting for the user.
        /// @param window The window that the dialog belongs to, or `nullptr` for none.
        /// @param defaultLocation The folder shown first, or an empty path to let the system choose.
        /// @param callback Called once when the dialog closes, which may happen before this function returns.
        virtual void ShowFolderDialog(const Window* window, const std::filesystem::path& defaultLocation, Callback callback) = 0;
    };

    /// @brief The native dialog of the system to choose a folder, shown without blocking the application.
    ///
    /// OpenFolder() shows the dialog and returns at once: the application keeps drawing its frames while the user chooses.
    /// The chosen folder is read on one of the next frames with TakeFolder(). The system may give the answer on another
    /// thread, and the object can still be read from the main thread at any time. A dialog destroyed while it is shown
    /// ignores its answer.
    class FileDialog
    {
    public:
        /// @brief Creates a closed dialog.
        /// @param backend Where the dialogs are shown, or nullptr for the dialogs of the system.
        explicit FileDialog(Scope<FileDialogBackend> backend = nullptr);
        ~FileDialog();

        FileDialog(const FileDialog&) = delete;
        FileDialog& operator=(const FileDialog&) = delete;
        FileDialog(FileDialog&&) = delete;
        FileDialog& operator=(FileDialog&&) = delete;

        /// @brief Shows the dialog to choose one folder, unless the dialog is already shown.
        /// @param window The window that the dialog belongs to, or `nullptr` for none.
        /// @param defaultLocation The folder shown first, or an empty path to let the system choose.
        void OpenFolder(const Window* window, const std::filesystem::path& defaultLocation);

        /// @brief Tells if the dialog is shown and waits for the user.
        bool IsOpen() const;

        /// @brief Returns the folder chosen by the user, once.
        /// @return The folder, as the system gives it, or no value if the dialog is still shown, was canceled, could not
        /// be shown, or if the folder was already read. When the dialog cannot be shown, the reason is logged as an engine
        /// error.
        [[nodiscard]] std::optional<std::filesystem::path> TakeFolder();

    private:
        struct State;

        std::shared_ptr<State> m_State;
        Scope<FileDialogBackend> m_Backend;
    };
}
