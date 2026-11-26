#pragma once

#include <EditorPanel.h>

#include <quasar/time/Timestep.h>

#include <filesystem>
#include <vector>

namespace Quasar::Editor
{
    struct EditorContext;

    struct ContentBrowserEntry
    {
        std::filesystem::path Path;
        bool IsDirectory = false;
    };

    struct ContentBrowserFolder
    {
        std::filesystem::path Path;
        int Depth = 0;
        bool HasChildren = false;
    };

    class ContentBrowserPanel final : public EditorPanel
    {
    public:
        explicit ContentBrowserPanel(EditorContext& context);

        void OnUpdate(Timestep timestep) override;
        bool OpenDirectory(const std::filesystem::path& directory);

        const std::filesystem::path& GetShownDirectory() const { return m_ShownDirectory; }
        const std::vector<ContentBrowserEntry>& GetEntries() const { return m_Entries; }
        const std::filesystem::path& GetSelected() const { return m_Selected; }
        const std::vector<ContentBrowserFolder>& GetFolders() const { return m_Folders; }

    protected:
        void OnImGuiRender() override;

    private:
        void FollowProject();
        void ReadShownDirectory();
        void DrawToolbar();
        void DrawEntries();
        void DrawFolders();

        std::filesystem::path m_ProjectDirectory;
        std::filesystem::path m_AssetsDirectory;
        std::filesystem::path m_ShownDirectory;
        std::vector<ContentBrowserEntry> m_Entries;
        std::filesystem::path m_Selected;
        std::vector<ContentBrowserFolder> m_Folders;
        bool m_RevealShownDirectory = false;
    };
}
