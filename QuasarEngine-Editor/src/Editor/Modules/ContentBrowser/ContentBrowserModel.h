#pragma once

#include <filesystem>
#include <memory>
#include <vector>
#include <string>
#include <optional>
#include <unordered_set>
#include <unordered_map>
#include <deque>
#include <future>

#include <QuasarEngine/Asset/Asset.h>
#include <QuasarEngine/Resources/Texture2D.h>

namespace QuasarEngine
{
    class ContentBrowserModel
    {
    public:
        enum class ClipMode { None, Copy, Cut };
        enum class SortMode { NameAsc, NameDesc, DateAsc, DateDesc };

        enum class EntryGroup {
            Folder,
            Texture,
            Model,
            Script,
            Scene,
            Particle,
            Other
        };

        struct Clipboard {
            ClipMode mode = ClipMode::None;
            std::vector<std::filesystem::path> items;
        };

        struct Entry
        {
            std::filesystem::path absPath{};
            std::string key;
            std::string filename;
            std::string filenameLower;
            std::string displayName;
            std::string ext;
            std::string extLower;

            bool isDir = false;
            bool isHidden = false;

            AssetType type = AssetType::NONE;
            EntryGroup group = EntryGroup::Other;

            uint64_t size = 0;
            std::filesystem::file_time_type modified{};

            std::shared_ptr<Texture2D> icon;

            bool isModel = false;
            std::string modelId;

            bool isTexture = false;
            std::string textureId;
        };

        struct IconSet
        {
            std::shared_ptr<Texture2D> directory;
            std::shared_ptr<Texture2D> png;
            std::shared_ptr<Texture2D> jpg;
            std::shared_ptr<Texture2D> obj;
            std::shared_ptr<Texture2D> lua;
            std::shared_ptr<Texture2D> scene;
            std::shared_ptr<Texture2D> other;
        };

        struct UIState
        {
            bool listView = false;
            bool showHidden = false;
            bool groupByType = false;
            bool watchEnabled = false;
            bool alwaysShowFolders = true;

            float thumbSize = 96.0f;

            SortMode sortMode = SortMode::NameAsc;
            int typeFilter = 0;

            char search[128]{};
            char path[512]{};

            float leftPaneWidth = 260.0f;
            float splitterWidth = 4.0f;

            double watchIntervalSec = 0.5;
        };

        struct DirSignature
        {
            std::filesystem::file_time_type maxWrite{};
            uint64_t count = 0;

            bool operator==(const DirSignature& o) const { return count == o.count && maxWrite == o.maxWrite; }
            bool operator!=(const DirSignature& o) const { return !(*this == o); }
        };

    public:
        explicit ContentBrowserModel(std::filesystem::path baseDir);

        void SetIconSet(IconSet icons);
        void SetTrashDirectory(std::filesystem::path trashDir);

        void Update(double dt);

        bool ChangeDirectory(const std::filesystem::path& dir);
        const std::filesystem::path& BaseDirectory() const { return m_BaseDirectory; }
        const std::filesystem::path& CurrentDirectory() const { return m_CurrentDirectory; }
        const std::filesystem::path& TrashDirectory() const { return m_TrashDir; }

        UIState& UI() { return m_UI; }
        const UIState& UI() const { return m_UI; }

        const std::vector<Entry>& Entries() const { return m_Entries; }
        const std::vector<size_t>& Visible() const { return m_Visible; }

        bool IsSelected(const std::string& key) const;
        void ClearSelection();
        void SelectSingle(const std::string& key);
        void ToggleSelection(const std::string& key);
        const std::unordered_set<std::string>& Selection() const { return m_Selection; }
        std::optional<std::string> SingleSelectionKey() const;

        void BeginRename(const std::filesystem::path& absPath);
        void CancelRename();
        const std::optional<std::filesystem::path>& RenamingPath() const { return m_RenamingPath; }
        char* RenameBuffer() { return m_RenameBuffer; }
        size_t RenameBufferSize() const { return sizeof(m_RenameBuffer); }

        void RequestDelete(std::vector<std::filesystem::path> paths);
        bool ConsumeOpenDeleteConfirm();
        const std::vector<std::filesystem::path>& PendingDelete() const { return m_PendingDelete; }
        void ClearPendingDelete();

        void SetLastError(std::string err);
        const std::string& LastError() const { return m_LastError; }

        void MarkCacheDirty();
        bool CacheDirty() const { return m_CacheDirty; }
        void RefreshDirectoryCache();
        void RebuildVisibleList();

        void RequestDirSizeForKey(const std::string& key, const std::filesystem::path& p);
        bool TryGetDirSizeCached(const std::string& key, uint64_t& outSize) const;
        bool IsDirSizeComputing(const std::string& key) const;
        void PollDirSizeJob();

        void RebuildTextureQueueFromVisible();
        bool PopNextTextureToLoad(std::string& outTextureId, std::filesystem::path& outAbsPath);
        void MarkTextureLoadRequested(const std::string& textureId);
        void EnqueueTextureIconUpdate(const std::string& textureId);
        void UpdateTextureIconsBudget(int budget);

        static std::filesystem::path UniqueNameInDir(const std::filesystem::path& dir, const std::string& base);
        static std::string NormalizeKey(const std::filesystem::path& p);
        static std::string PrettySize(uint64_t s);
        static std::time_t ToTimeT(std::filesystem::file_time_type tp);

        static EntryGroup ComputeGroup(bool isDir, AssetType t);
        static const char* GroupLabel(EntryGroup g);
        static int GroupOrder(EntryGroup g);

    private:
        static std::string ToLowerCopy(const std::string& s);
        static uint64_t DirSize(const std::filesystem::path& p);
        static DirSignature ComputeDirSignature(const std::filesystem::path& dir, bool showHidden, const std::vector<std::string>& ignoredExt);

        void FillEntryMetadata(Entry& e);
        bool PassesSearchFilter(const Entry& e, const std::string& filterLower) const;
        bool PassesTypeFilter(const Entry& e) const;

    private:
        std::filesystem::path m_BaseDirectory;
        std::filesystem::path m_CurrentDirectory;
        std::filesystem::path m_TrashDir;

        IconSet m_Icons{};
        UIState m_UI{};

        std::vector<std::string> m_IgnoredExtensions = { ".ultconf", ".meta" };

        std::unordered_set<std::string> m_Selection;
        Clipboard m_Clipboard;

        std::optional<std::filesystem::path> m_RenamingPath;
        char m_RenameBuffer[256]{};

        std::vector<std::filesystem::path> m_PendingDelete;
        bool m_OpenDeleteConfirm = false;

        std::string m_LastError;

        bool m_CacheDirty = true;
        std::vector<Entry> m_Entries;
        std::vector<size_t> m_Visible;

        double m_WatchAccum = 0.0;
        DirSignature m_CurrentSig{};

        std::deque<std::string> m_TextureLoadQueue;
        std::unordered_set<std::string> m_TextureLoadRequested;
        std::unordered_map<std::string, std::filesystem::path> m_TextureIdToPath;
        std::unordered_map<std::string, size_t> m_TextureIdToEntryIndex;

        std::deque<std::string> m_TextureIconUpdateQueue;

        std::unordered_map<std::string, bool> m_ExpandedModels;
    public:
        std::unordered_map<std::string, bool>& ExpandedModels() { return m_ExpandedModels; }
        const std::unordered_map<std::string, bool>& ExpandedModels() const { return m_ExpandedModels; }

        Clipboard& ClipboardState() { return m_Clipboard; }
        const Clipboard& ClipboardState() const { return m_Clipboard; }

        const std::vector<std::string>& IgnoredExtensions() const { return m_IgnoredExtensions; }
    public:
        struct AsyncSizeJob {
            std::string key;
            std::filesystem::path path;
            std::future<uint64_t> fut;
        };
    private:
        std::optional<AsyncSizeJob> m_PreviewSizeJob;
        std::unordered_map<std::string, uint64_t> m_DirSizeCache;
    };
}