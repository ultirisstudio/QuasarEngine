#pragma once

#include "FolderDialog.h"

#include <array>
#include <filesystem>
#include <optional>
#include <string>

namespace Quasar::Editor
{
    class ProjectPopups
    {
    public:
        void RequestNewProject();

        void RequestOpenProject();

        [[nodiscard]] std::optional<std::filesystem::path> Draw();

    private:
        using TextBuffer = std::array<char, 512>;

        const std::filesystem::path& GetLastLocation();

        std::optional<std::filesystem::path> DrawNewProjectPopup();
        static void DrawNewProjectHeader();
        void DrawNewProjectBody() const;
        std::optional<std::filesystem::path> DrawNewProjectFooter();

        std::optional<std::filesystem::path> ReceiveOpenedProject();
        void ShowOpenError(std::string error);
        void DrawOpenErrorPopup();

        bool m_NewProjectRequested = false;
        TextBuffer m_Name{};
        TextBuffer m_Location{};
        std::string m_NewProjectError;
        std::string m_OpenError;
        std::filesystem::path m_LastLocation;
        FolderDialog m_LocationDialog;
        FolderDialog m_ProjectDialog;
    };
}
