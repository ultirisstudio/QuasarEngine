#pragma once

/// \file
/// \brief The native dialog of the system to choose a folder.
/// \author Fabien Wahbeh
/// \date 2026-11-23

#include <filesystem>
#include <memory>
#include <optional>

namespace Quasar
{
    class Window;

    /// @brief The native dialog of the system to choose a folder, shown without blocking the application.
    ///
    /// OpenFolder() shows the dialog and returns at once: the application keeps drawing its frames while the user chooses.
    /// The chosen folder is read on one of the next frames with TakeFolder(). The system may give the answer on another
    /// thread, and the object can still be read from the main thread at any time.
    class FileDialog
    {
    public:
        FileDialog();
        ~FileDialog() = default;

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

        static void OnFolderChosen(void* userdata, const char* const* fileList, int filter);

        std::shared_ptr<State> m_State;
    };
}
