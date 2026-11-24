#pragma once

#include "EditorSettings.h"
#include "Project.h"

#include <quasar/core/Base.h>
#include <quasar/io/FileDialog.h>

#include <cstdint>
#include <filesystem>
#include <optional>
#include <string>

namespace Quasar::Editor
{
    class ProjectPopups
    {
    public:
        explicit ProjectPopups(EditorSettings& settings, Scope<FileDialogBackend> dialogBackend = nullptr);

        void SetParentWindow(const Window* window) { m_ParentWindow = window; }

        void RequestNewProject();

        void RequestOpenProject();

        void RequestOpenProject(std::filesystem::path projectDirectory);

        [[nodiscard]] std::optional<ProjectResult> Draw();

    private:
        enum class DialogPurpose : std::uint8_t
        {
            None,
            Location,
            Project
        };

        std::filesystem::path GetLastLocation() const;
        void OpenDialog(DialogPurpose purpose, const std::filesystem::path& location);
        void ReceiveDialogFolder();

        std::optional<ProjectResult> DrawNewProjectPopup();
        static void DrawNewProjectHeader();
        void DrawNewProjectBody() const;
        std::optional<ProjectResult> DrawNewProjectFooter();

        std::optional<ProjectResult> OpenRequestedProject();
        void DrawOpenErrorPopup();

        EditorSettings* m_Settings;
        const Window* m_ParentWindow = nullptr;
        FileDialog m_Dialog;
        DialogPurpose m_DialogPurpose = DialogPurpose::None;
        bool m_NewProjectRequested = false;
        std::optional<std::filesystem::path> m_ProjectToOpen;
        std::string m_Name;
        std::string m_Location;
        std::string m_NewProjectError;
        std::string m_OpenError;
    };
}
