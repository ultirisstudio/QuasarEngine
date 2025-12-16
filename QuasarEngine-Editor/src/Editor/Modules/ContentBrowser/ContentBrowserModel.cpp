#include "ContentBrowserModel.h"

#include <algorithm>
#include <cctype>
#include <chrono>
#include <cstdio>
#include <cstring>
#include <system_error>

#include <QuasarEngine/Scene/Importer/TextureConfigImporter.h>
#include <QuasarEngine/Asset/Asset.h>
#include <QuasarEngine/Core/Logger.h>
#include <QuasarEngine/Asset/AssetManager.h>

namespace QuasarEngine
{
    ContentBrowserModel::ContentBrowserModel(std::filesystem::path baseDir)
        : m_BaseDirectory(std::move(baseDir))
        , m_CurrentDirectory(m_BaseDirectory)
    {
        std::snprintf(m_UI.path, sizeof(m_UI.path), "%s", m_CurrentDirectory.string().c_str());
        MarkCacheDirty();
    }

    void ContentBrowserModel::SetIconSet(IconSet icons)
    {
        m_Icons = std::move(icons);
        MarkCacheDirty();
    }

    void ContentBrowserModel::SetTrashDirectory(std::filesystem::path trashDir)
    {
        m_TrashDir = std::move(trashDir);
    }

    void ContentBrowserModel::SetLastError(std::string err)
    {
        m_LastError = std::move(err);
    }

    void ContentBrowserModel::MarkCacheDirty()
    {
        m_CacheDirty = true;
    }

    bool ContentBrowserModel::ChangeDirectory(const std::filesystem::path& dir)
    {
        std::error_code ec;
        if (!std::filesystem::exists(dir, ec) || !std::filesystem::is_directory(dir, ec))
            return false;

        m_CurrentDirectory = dir.lexically_normal();
        std::snprintf(m_UI.path, sizeof(m_UI.path), "%s", m_CurrentDirectory.string().c_str());

        ClearSelection();
        CancelRename();
        MarkCacheDirty();
        return true;
    }

    bool ContentBrowserModel::IsSelected(const std::string& key) const
    {
        return m_Selection.find(key) != m_Selection.end();
    }

    void ContentBrowserModel::ClearSelection()
    {
        m_Selection.clear();
    }

    void ContentBrowserModel::SelectSingle(const std::string& key)
    {
        m_Selection.clear();
        m_Selection.insert(key);
    }

    void ContentBrowserModel::ToggleSelection(const std::string& key)
    {
        if (IsSelected(key)) m_Selection.erase(key);
        else m_Selection.insert(key);
    }

    std::optional<std::string> ContentBrowserModel::SingleSelectionKey() const
    {
        if (m_Selection.size() != 1) return std::nullopt;
        return *m_Selection.begin();
    }

    void ContentBrowserModel::BeginRename(const std::filesystem::path& absPath)
    {
        m_RenamingPath = absPath;
        std::string filename = absPath.filename().string();
        std::memset(m_RenameBuffer, 0, sizeof(m_RenameBuffer));
        std::strncpy(m_RenameBuffer, filename.c_str(), sizeof(m_RenameBuffer) - 1);
    }

    void ContentBrowserModel::CancelRename()
    {
        m_RenamingPath.reset();
        std::memset(m_RenameBuffer, 0, sizeof(m_RenameBuffer));
    }

    void ContentBrowserModel::RequestDelete(std::vector<std::filesystem::path> paths)
    {
        m_PendingDelete = std::move(paths);
        m_OpenDeleteConfirm = !m_PendingDelete.empty();
    }

    bool ContentBrowserModel::ConsumeOpenDeleteConfirm()
    {
        bool v = m_OpenDeleteConfirm;
        m_OpenDeleteConfirm = false;
        return v;
    }

    void ContentBrowserModel::ClearPendingDelete()
    {
        m_PendingDelete.clear();
    }

    std::string ContentBrowserModel::ToLowerCopy(const std::string& s)
    {
        std::string out;
        out.resize(s.size());
        for (size_t i = 0; i < s.size(); ++i)
            out[i] = (char)std::tolower((unsigned char)s[i]);
        return out;
    }

    std::filesystem::path ContentBrowserModel::UniqueNameInDir(const std::filesystem::path& dir, const std::string& base)
    {
        auto candidate = dir / base;
        if (!std::filesystem::exists(candidate)) return candidate;

        int i = 1;
        while (true) {
            auto c = dir / (base + " (" + std::to_string(i++) + ")");
            if (!std::filesystem::exists(c)) return c;
        }
    }

    std::string ContentBrowserModel::NormalizeKey(const std::filesystem::path& p)
    {
        std::error_code ec;
        auto wc = std::filesystem::weakly_canonical(p, ec);
        if (!ec) return wc.generic_string();
        return p.lexically_normal().generic_string();
    }

    std::string ContentBrowserModel::PrettySize(uint64_t s)
    {
        const char* u[] = { "B","KB","MB","GB","TB" };
        int i = 0;
        double d = (double)s;
        while (d > 1024.0 && i < 4) { d /= 1024.0; ++i; }
        char b[32];
        std::snprintf(b, sizeof(b), "%.1f %s", d, u[i]);
        return b;
    }

    std::time_t ContentBrowserModel::ToTimeT(std::filesystem::file_time_type tp)
    {
        using namespace std::chrono;
        auto sctp = time_point_cast<system_clock::duration>(tp - decltype(tp)::clock::now() + system_clock::now());
        return system_clock::to_time_t(sctp);
    }

    uint64_t ContentBrowserModel::DirSize(const std::filesystem::path& p)
    {
        uint64_t s = 0;
        std::error_code ec;
        for (auto& e : std::filesystem::recursive_directory_iterator(p, ec)) {
            if (ec) break;
            if (!e.is_directory(ec)) {
                s += (uint64_t)std::filesystem::file_size(e.path(), ec);
                if (ec) ec.clear();
            }
        }
        return s;
    }

    ContentBrowserModel::DirSignature ContentBrowserModel::ComputeDirSignature(
        const std::filesystem::path& dir,
        bool showHidden,
        const std::vector<std::string>& ignoredExt)
    {
        DirSignature sig{};
        std::error_code ec;

        for (auto it = std::filesystem::directory_iterator(dir, ec);
            !ec && it != std::filesystem::directory_iterator(); ++it)
        {
            const auto p = it->path();
            const std::string name = p.filename().string();
            if (!showHidden && !name.empty() && name[0] == '.') continue;

            if (it->is_regular_file(ec)) {
                std::string ext = p.extension().string();
                std::string extLower = ToLowerCopy(ext);
                bool ignore = false;
                for (const auto& ign : ignoredExt) {
                    if (extLower == ign) { ignore = true; break; }
                }
                if (ignore) continue;
            }

            ++sig.count;

            std::error_code ec2;
            auto tp = std::filesystem::last_write_time(p, ec2);
            if (!ec2) {
                if (sig.count == 1) sig.maxWrite = tp;
                else sig.maxWrite = std::max(sig.maxWrite, tp);
            }
        }

        return sig;
    }

    ContentBrowserModel::EntryGroup ContentBrowserModel::ComputeGroup(bool isDir, AssetType t)
    {
        if (isDir) return EntryGroup::Folder;

        switch (t)
        {
        case AssetType::TEXTURE:  return EntryGroup::Texture;
        case AssetType::SCRIPT:   return EntryGroup::Script;
        case AssetType::SCENE:    return EntryGroup::Scene;
        case AssetType::MODEL:    return EntryGroup::Model;
        case AssetType::MESH:     return EntryGroup::Model;
        case AssetType::PARTICLE: return EntryGroup::Particle;
        default:                  return EntryGroup::Other;
        }
    }

    const char* ContentBrowserModel::GroupLabel(EntryGroup g)
    {
        switch (g)
        {
        case EntryGroup::Folder:   return "Folders";
        case EntryGroup::Texture:  return "Textures";
        case EntryGroup::Model:    return "Meshes / Models";
        case EntryGroup::Script:   return "Scripts";
        case EntryGroup::Scene:    return "Scenes";
        case EntryGroup::Particle: return "Particles";
        default:                   return "Other";
        }
    }

    int ContentBrowserModel::GroupOrder(EntryGroup g)
    {
        switch (g)
        {
        case EntryGroup::Folder:   return 0;
        case EntryGroup::Texture:  return 1;
        case EntryGroup::Model:    return 2;
        case EntryGroup::Script:   return 3;
        case EntryGroup::Scene:    return 4;
        case EntryGroup::Particle: return 5;
        default:                   return 6;
        }
    }

    void ContentBrowserModel::FillEntryMetadata(Entry& e)
    {
        std::error_code ec;

        e.filename = e.absPath.filename().string();
        e.filenameLower = ToLowerCopy(e.filename);

        e.ext = e.absPath.extension().string();
        e.extLower = ToLowerCopy(e.ext);

        e.isDir = std::filesystem::is_directory(e.absPath, ec);
        e.key = NormalizeKey(e.absPath);

        e.isHidden = (!e.filename.empty() && e.filename[0] == '.');

        {
            auto stem = e.absPath.stem().string();
            e.displayName = stem.empty() ? e.filename : stem;
        }

        if (e.isDir) e.type = AssetType::NONE;
        else e.type = AssetManager::Instance().getTypeFromExtention(e.ext);

        e.group = ComputeGroup(e.isDir, e.type);

        e.modified = std::filesystem::last_write_time(e.absPath, ec);

        if (!e.isDir) {
            ec.clear();
            auto sz = std::filesystem::file_size(e.absPath, ec);
            e.size = ec ? 0 : (uint64_t)sz;
        }
        else {
            e.size = 0;
        }

        e.icon = m_Icons.other;
        e.isModel = false;
        e.modelId.clear();
        e.isTexture = false;
        e.textureId.clear();

        if (e.isDir) {
            e.icon = m_Icons.directory;
            return;
        }

        if (e.type == AssetType::MODEL || e.type == AssetType::MESH) {
            e.icon = m_Icons.obj;
            e.isModel = true;

            std::error_code ecRel;
            if (auto rel = std::filesystem::relative(e.absPath, m_BaseDirectory, ecRel); !ecRel)
                e.modelId = "Assets/" + rel.generic_string();
            else
                e.modelId = "Assets/" + e.absPath.filename().generic_string();

            return;
        }

        if (e.type == AssetType::TEXTURE) {
            e.isTexture = true;

            std::error_code ecRel;
            if (auto rel = std::filesystem::relative(e.absPath, m_BaseDirectory, ecRel); !ecRel)
                e.textureId = "Assets/" + rel.generic_string();
            else
                e.textureId = "Assets/" + e.absPath.filename().generic_string();

            if (!e.textureId.empty() && AssetManager::Instance().isAssetLoaded(e.textureId)) {
                auto tex = AssetManager::Instance().getAsset<Texture2D>(e.textureId);
                e.icon = tex ? tex : m_Icons.other;
            }
            else {
                if (e.extLower == ".png") e.icon = m_Icons.png;
                else if (e.extLower == ".jpg" || e.extLower == ".jpeg") e.icon = m_Icons.jpg;
                else e.icon = m_Icons.other;
            }
            return;
        }

        if (e.type == AssetType::SCRIPT) {
            e.icon = (e.extLower == ".lua") ? m_Icons.lua : m_Icons.other;
            return;
        }

        if (e.type == AssetType::SCENE) {
            e.icon = m_Icons.scene;
            return;
        }

        e.icon = m_Icons.other;
    }

    bool ContentBrowserModel::PassesSearchFilter(const Entry& e, const std::string& filterLower) const
    {
        if (filterLower.empty()) return true;
        return e.filenameLower.find(filterLower) != std::string::npos;
    }

    bool ContentBrowserModel::PassesTypeFilter(const Entry& e) const
    {
        const int typeFilter = m_UI.typeFilter;
        if (typeFilter == 0) return true;

        if (m_UI.alwaysShowFolders && e.isDir) return true;

        if (typeFilter == 1) return e.type == AssetType::TEXTURE;
        if (typeFilter == 2) return (e.type == AssetType::MODEL || e.type == AssetType::MESH);
        if (typeFilter == 3) return e.type == AssetType::SCRIPT;
        if (typeFilter == 4) return e.type == AssetType::SCENE;
        if (typeFilter == 5) {
            return !(e.type == AssetType::TEXTURE ||
                e.type == AssetType::MODEL ||
                e.type == AssetType::MESH ||
                e.type == AssetType::SCRIPT ||
                e.type == AssetType::SCENE);
        }
        return true;
    }

    void ContentBrowserModel::RefreshDirectoryCache()
    {
        m_CacheDirty = false;
        m_LastError.clear();

        m_Entries.clear();
        m_Visible.clear();

        m_TextureLoadQueue.clear();
        m_TextureIdToPath.clear();
        m_TextureIdToEntryIndex.clear();
        m_TextureIconUpdateQueue.clear();
        m_TextureLoadRequested.clear();

        std::error_code ecIter;

        for (auto it = std::filesystem::directory_iterator(m_CurrentDirectory, ecIter);
            !ecIter && it != std::filesystem::directory_iterator(); ++it)
        {
            Entry e;
            e.absPath = it->path();
            FillEntryMetadata(e);

            if (!e.isDir) {
                bool ignore = false;
                for (const auto& ign : m_IgnoredExtensions) {
                    if (e.extLower == ign) { ignore = true; break; }
                }
                if (ignore) continue;
            }

            if (e.isTexture && !e.textureId.empty()) {
                m_TextureIdToPath[e.textureId] = e.absPath;
                m_TextureIdToEntryIndex[e.textureId] = m_Entries.size();
            }

            m_Entries.push_back(std::move(e));
        }

        m_CurrentSig = ComputeDirSignature(m_CurrentDirectory, m_UI.showHidden, m_IgnoredExtensions);

        RebuildVisibleList();
        RebuildTextureQueueFromVisible();
    }

    void ContentBrowserModel::RebuildVisibleList()
    {
        m_Visible.clear();

        const std::string filterLower = ToLowerCopy(std::string(m_UI.search));

        for (size_t i = 0; i < m_Entries.size(); ++i)
        {
            const Entry& e = m_Entries[i];

            if (!m_UI.showHidden && e.isHidden) continue;
            if (!PassesSearchFilter(e, filterLower)) continue;
            if (!PassesTypeFilter(e)) continue;

            m_Visible.push_back(i);
        }

        auto cmp = [&](size_t ia, size_t ib) -> bool
            {
                const Entry& a = m_Entries[ia];
                const Entry& b = m_Entries[ib];

                if (a.isDir != b.isDir) return a.isDir > b.isDir;

                if (m_UI.groupByType) {
                    const int ga = GroupOrder(a.group);
                    const int gb = GroupOrder(b.group);
                    if (ga != gb) return ga < gb;
                }

                switch (m_UI.sortMode)
                {
                case SortMode::NameAsc:  return a.filename < b.filename;
                case SortMode::NameDesc: return a.filename > b.filename;
                case SortMode::DateAsc:  return a.modified < b.modified;
                case SortMode::DateDesc: return a.modified > b.modified;
                }
                return false;
            };

        std::stable_sort(m_Visible.begin(), m_Visible.end(), cmp);

        RebuildTextureQueueFromVisible();
    }

    void ContentBrowserModel::RebuildTextureQueueFromVisible()
    {
        for (size_t idx : m_Visible)
        {
            const Entry& e = m_Entries[idx];
            if (!e.isTexture || e.textureId.empty()) continue;
            if (AssetManager::Instance().isAssetLoaded(e.textureId)) continue;
            if (m_TextureLoadRequested.find(e.textureId) != m_TextureLoadRequested.end()) continue;

            bool already = false;
            for (const auto& q : m_TextureLoadQueue) { if (q == e.textureId) { already = true; break; } }
            if (!already) m_TextureLoadQueue.push_back(e.textureId);
        }
    }

    bool ContentBrowserModel::PopNextTextureToLoad(std::string& outTextureId, std::filesystem::path& outAbsPath)
    {
        while (!m_TextureLoadQueue.empty())
        {
            std::string id = m_TextureLoadQueue.front();
            m_TextureLoadQueue.pop_front();

            if (id.empty()) continue;
            if (m_TextureLoadRequested.find(id) != m_TextureLoadRequested.end()) continue;

            auto it = m_TextureIdToPath.find(id);
            if (it == m_TextureIdToPath.end()) continue;

            outTextureId = id;
            outAbsPath = it->second;
            return true;
        }
        return false;
    }

    void ContentBrowserModel::MarkTextureLoadRequested(const std::string& textureId)
    {
        if (!textureId.empty())
            m_TextureLoadRequested.insert(textureId);
    }

    void ContentBrowserModel::EnqueueTextureIconUpdate(const std::string& textureId)
    {
        if (textureId.empty()) return;
        m_TextureIconUpdateQueue.push_back(textureId);
    }

    void ContentBrowserModel::UpdateTextureIconsBudget(int budget)
    {
        while (budget-- > 0 && !m_TextureIconUpdateQueue.empty())
        {
            std::string id = m_TextureIconUpdateQueue.front();
            m_TextureIconUpdateQueue.pop_front();

            auto itEntry = m_TextureIdToEntryIndex.find(id);
            if (itEntry == m_TextureIdToEntryIndex.end()) continue;

            if (!AssetManager::Instance().isAssetLoaded(id)) {
                m_TextureIconUpdateQueue.push_back(id);
                continue;
            }

            auto tex = AssetManager::Instance().getAsset<Texture2D>(id);
            if (!tex) continue;

            size_t idx = itEntry->second;
            if (idx >= m_Entries.size()) continue;

            Entry& e = m_Entries[idx];
            if (e.isTexture) e.icon = tex;
        }
    }

    void ContentBrowserModel::RequestDirSizeForKey(const std::string& key, const std::filesystem::path& p)
    {
        if (key.empty()) return;
        if (m_DirSizeCache.find(key) != m_DirSizeCache.end()) return;
        if (m_PreviewSizeJob && m_PreviewSizeJob->key == key) return;

        AsyncSizeJob job;
        job.key = key;
        job.path = p;
        job.fut = std::async(std::launch::async, [p]() { return DirSize(p); });
        m_PreviewSizeJob = std::move(job);
    }

    bool ContentBrowserModel::TryGetDirSizeCached(const std::string& key, uint64_t& outSize) const
    {
        auto it = m_DirSizeCache.find(key);
        if (it == m_DirSizeCache.end()) return false;
        outSize = it->second;
        return true;
    }

    bool ContentBrowserModel::IsDirSizeComputing(const std::string& key) const
    {
        return m_PreviewSizeJob && m_PreviewSizeJob->key == key;
    }

    void ContentBrowserModel::PollDirSizeJob()
    {
        if (!m_PreviewSizeJob) return;

        auto status = m_PreviewSizeJob->fut.wait_for(std::chrono::milliseconds(0));
        if (status != std::future_status::ready) return;

        uint64_t result = 0;
        try { result = m_PreviewSizeJob->fut.get(); }
        catch (...) { result = 0; }

        m_DirSizeCache[m_PreviewSizeJob->key] = result;
        m_PreviewSizeJob.reset();
    }

    void ContentBrowserModel::Update(double dt)
    {
        if (m_UI.watchEnabled)
        {
            m_WatchAccum += dt;
            if (m_WatchAccum >= m_UI.watchIntervalSec)
            {
                m_WatchAccum = 0.0;
                DirSignature sig = ComputeDirSignature(m_CurrentDirectory, m_UI.showHidden, m_IgnoredExtensions);
                if (sig != m_CurrentSig) {
                    m_CurrentSig = sig;
                    MarkCacheDirty();
                }
            }
        }

        if (m_CacheDirty)
            RefreshDirectoryCache();

        PollDirSizeJob();
    }
}