#pragma once

#include <cstddef>
#include <filesystem>
#include <vector>

namespace Quasar::Editor
{
    inline constexpr std::size_t MaxRecentProjects = 10;

    class EditorSettings
    {
    public:
        // Reads the file if it exists. An empty path keeps the settings in memory only.
        explicit EditorSettings(std::filesystem::path file = {});

        const std::filesystem::path& GetFile() const { return m_File; }

        const std::filesystem::path& GetLastLocation() const { return m_LastLocation; }

        const std::vector<std::filesystem::path>& GetRecentProjects() const { return m_RecentProjects; }

        void RememberProject(const std::filesystem::path& projectDirectory);

        void ForgetProject(const std::filesystem::path& projectDirectory);

    private:
        void Load();
        void Save() const;

        std::filesystem::path m_File;
        std::filesystem::path m_LastLocation;
        std::vector<std::filesystem::path> m_RecentProjects;
    };

    std::filesystem::path GetEditorSettingsFile();
}
