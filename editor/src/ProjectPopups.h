#pragma once

#include <quasar/io/FileDialog.h>

#include <filesystem>
#include <optional>
#include <string>

namespace Quasar::Editor
{
    class ProjectPopups
    {
    public:
        explicit ProjectPopups(std::filesystem::path defaultLocation = {});

        void RequestNewProject();

        void RequestOpenProject();

        [[nodiscard]] std::optional<std::filesystem::path> Draw();

    private:
        const std::filesystem::path& GetLastLocation();

        std::optional<std::filesystem::path> DrawNewProjectPopup();
        static void DrawNewProjectHeader();
        void DrawNewProjectBody() const;
        std::optional<std::filesystem::path> DrawNewProjectFooter();

        std::optional<std::filesystem::path> ReceiveOpenedProject();
        void ShowOpenError(std::string error);
        void DrawOpenErrorPopup();

        bool m_NewProjectRequested = false;
        std::string m_Name;
        std::string m_Location;
        std::string m_NewProjectError;
        std::string m_OpenError;
        std::filesystem::path m_LastLocation;
        FileDialog m_LocationDialog;
        FileDialog m_ProjectDialog;
    };
}
