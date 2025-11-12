#pragma once

#include <filesystem>
#include <memory>
#include <vector>
#include <string>
#include <optional>
#include <unordered_set>

#include <Editor/Modules/TextureViewer/TextureViewer.h>
#include <Editor/Modules/CodeEditor/CodeEditor.h>

#include <QuasarEngine/Resources/Texture2D.h>
#include <QuasarEngine/Scene/Importer/AssetImporter.h>

namespace QuasarEngine
{
    class ContentBrowser
    {
    public:
        ContentBrowser(const std::string& projectPath, AssetImporter* importer);
        ~ContentBrowser();

        void Update();
        void OnImGuiRender();

    private:
        const std::string GetFileExtension(std::filesystem::directory_entry e);
        static std::filesystem::path UniqueNameInDir(const std::filesystem::path& dir, const std::string& base);
        static bool MoveToTrash(const std::filesystem::path& p, const std::filesystem::path& trash, std::string& err);
        static bool CopyRec(const std::filesystem::path& src, const std::filesystem::path& dst, std::error_code& ec);
        static std::string PrettySize(uint64_t s);
        static uint64_t DirSize(const std::filesystem::path& p);
        static std::time_t ToTimeT(std::filesystem::file_time_type tp);

        void DrawToolbar();
        void DrawBreadcrumbs();
        void DrawListHeaderIfNeeded();
        void DrawPreviewPane(float rightPaneWidth);
        void HandleKeyboardShortcuts(bool windowFocused);
        void OpenRenameForSingleSelection();

        void DrawFolderTreePanel();
        void DrawFolderNode(const std::filesystem::path& dir);

        bool PassesSearchFilter(const std::string& filename, const std::string& filter) const;
        bool PassesTypeFilter(int typeFilter, int assetTypeValue) const;

        std::filesystem::path m_BaseDirectory;
        std::filesystem::path m_CurrentDirectory;

        std::shared_ptr<Texture2D> m_DirectoryIcon;
        std::shared_ptr<Texture2D> m_FilePNGIcon;
        std::shared_ptr<Texture2D> m_FileJPGIcon;
        std::shared_ptr<Texture2D> m_FileOBJIcon;
        std::shared_ptr<Texture2D> m_FileLuaIcon;
        std::shared_ptr<Texture2D> m_FileSceneIcon;
        std::shared_ptr<Texture2D> m_FileOtherIcon;

        std::shared_ptr<TextureViewer> m_TextureViewer;
        std::shared_ptr<CodeEditor>    m_CodeEditor;

        std::filesystem::path m_TrashDir;
        std::vector<std::filesystem::path> m_PendingDelete;
        bool        m_ShowConfirmDelete = false;
        std::string m_LastError;

        std::optional<std::filesystem::path> m_RenamingPath;
        char m_RenameBuffer[256]{};

        enum class ClipMode { None, Copy, Cut };
        struct Clipboard {
            ClipMode mode = ClipMode::None;
            std::vector<std::filesystem::path> items;
        } m_Clipboard;

        AssetImporter* m_AssetImporter = nullptr;

        bool  m_ListView = false;
        bool  m_ShowHidden = false;
        bool  m_GroupByType = false;
        bool  m_WatchEnabled = false;
        float m_ThumbSize = 96.0f;

        enum class SortMode { NameAsc, NameDesc, DateAsc, DateDesc };
        SortMode m_SortMode = SortMode::NameAsc;

        int   m_TypeFilter = 0;
        char  m_SearchBuffer[128]{};
        char  m_PathBuffer[512]{};

        std::unordered_set<std::string> m_Selection;

        std::vector<std::string> m_IgnoredExtensions = { ".ultconf", ".meta" };

        std::unordered_map<std::string, bool> m_ExpandedModels;

        float m_LeftPaneWidth = 260.0f;
        float m_SplitterWidth = 4.0f;
    };
}