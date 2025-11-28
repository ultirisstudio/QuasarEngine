#include "ContentBrowser.h"

#include <iostream>
#include <fstream>
#include <unordered_map>
#include <cctype>
#include <chrono>
#include <ctime>

#include "imgui/imgui.h"

#include <QuasarEngine/Asset/Asset.h>
#include <QuasarEngine/Core/Logger.h>
#include <QuasarEngine/Scene/Importer/TextureConfigImporter.h>
#include <QuasarEngine/Resources/Model.h>

#include "Editor/Resources/images_data.h"
#include <imgui/imgui_internal.h>

namespace QuasarEngine
{
    std::filesystem::path ContentBrowser::UniqueNameInDir(const std::filesystem::path& dir, const std::string& base)
    {
        auto candidate = dir / base;
        if (!std::filesystem::exists(candidate)) return candidate;
        int i = 1;
        while (true) {
            auto c = dir / (base + " (" + std::to_string(i++) + ")");
            if (!std::filesystem::exists(c)) return c;
        }
    }

    bool ContentBrowser::MoveToTrash(const std::filesystem::path& p, const std::filesystem::path& trash, std::string& err)
    {
        std::error_code ec;
        auto target = UniqueNameInDir(trash, p.filename().string());
        std::filesystem::rename(p, target, ec);
        if (ec) { err = ec.message(); return false; }
        return true;
    }

    bool ContentBrowser::CopyRec(const std::filesystem::path& src, const std::filesystem::path& dst, std::error_code& ec)
    {
        if (std::filesystem::is_directory(src)) {
            std::filesystem::create_directories(dst, ec);
            if (ec) return false;
            for (auto& e : std::filesystem::directory_iterator(src))
                if (!CopyRec(e.path(), dst / e.path().filename(), ec)) return false;
            return true;
        }
        else {
            std::filesystem::create_directories(dst.parent_path(), ec);
            if (ec) return false;
            std::filesystem::copy_file(src, dst, std::filesystem::copy_options::overwrite_existing, ec);
            return !ec;
        }
    }

    std::string ContentBrowser::PrettySize(uint64_t s)
    {
        const char* u[] = { "B","KB","MB","GB","TB" }; int i = 0; double d = (double)s;
        while (d > 1024.0 && i < 4) { d /= 1024.0; ++i; }
        char b[32]; std::snprintf(b, sizeof(b), "%.1f %s", d, u[i]); return b;
    }

    uint64_t ContentBrowser::DirSize(const std::filesystem::path& p)
    {
        uint64_t s = 0; std::error_code ec;
        for (auto& e : std::filesystem::recursive_directory_iterator(p, ec))
            if (!e.is_directory()) s += (uint64_t)std::filesystem::file_size(e.path(), ec);
        return s;
    }

    std::time_t ContentBrowser::ToTimeT(std::filesystem::file_time_type tp)
    {
        using namespace std::chrono;
        auto sctp = time_point_cast<system_clock::duration>(tp - decltype(tp)::clock::now() + system_clock::now());
        return system_clock::to_time_t(sctp);
    }

    const std::string ContentBrowser::GetFileExtension(std::filesystem::directory_entry e)
    {
        return e.path().extension().string();
    }

	ContentBrowser::ContentBrowser(EditorContext& context) : IEditorModule(context)
        , m_BaseDirectory((std::filesystem::path(context.projectPath) / "Assets").lexically_normal())
        , m_CurrentDirectory(m_BaseDirectory)
    {
        TextureSpecification spec;

        m_DirectoryIcon = Texture2D::Create(spec);
        m_DirectoryIcon->LoadFromMemory({ img_texture_dossier, img_texture_dossier_size });

        m_FilePNGIcon = Texture2D::Create(spec);
        m_FilePNGIcon->LoadFromMemory({ img_texture_png, img_texture_png_size });

        m_FileJPGIcon = Texture2D::Create(spec);
        m_FileJPGIcon->LoadFromMemory({ img_texture_jpg, img_texture_jpg_size });

        m_FileOBJIcon = Texture2D::Create(spec);
        m_FileOBJIcon->LoadFromMemory({ img_texture_obj, img_texture_obj_size });

        m_FileSceneIcon = Texture2D::Create(spec);
        m_FileSceneIcon->LoadFromMemory({ img_texture_scene, img_texture_scene_size });

        m_FileOtherIcon = Texture2D::Create(spec);
        m_FileOtherIcon->LoadFromMemory({ img_texture_texte, img_texture_texte_size });

        m_FileLuaIcon = Texture2D::Create(spec);
        m_FileLuaIcon->LoadFromMemory({ img_texture_lua, img_texture_lua_size });

        m_TrashDir = m_BaseDirectory / ".Trash";
        std::error_code ec;
        std::filesystem::create_directories(m_TrashDir, ec);

        std::snprintf(m_PathBuffer, sizeof(m_PathBuffer), "%s", m_CurrentDirectory.string().c_str());
    }

    ContentBrowser::~ContentBrowser()
    {
        m_DirectoryIcon.reset();
        m_FilePNGIcon.reset();
        m_FileJPGIcon.reset();
        m_FileOBJIcon.reset();
        m_FileOtherIcon.reset();
    }

    void ContentBrowser::Update(double dt)
    {
        if (m_TextureViewer) m_TextureViewer->Update(dt);
        if (m_WatchEnabled) {
            
        }
    }

    void ContentBrowser::DrawBreadcrumbs()
    {
        std::filesystem::path p = m_CurrentDirectory.lexically_normal();

        std::filesystem::path root = p.root_path();
        std::filesystem::path accum = root;
        std::vector<std::filesystem::path> crumbs;

        if (!root.empty())
            crumbs.push_back(root);

        for (auto& part : p.relative_path())
        {
            accum /= part;
            crumbs.push_back(accum);
        }

        ImGui::Text("Path: ");
        ImGui::SameLine();

        for (size_t i = 0; i < crumbs.size(); ++i)
        {
            if (i > 0) ImGui::SameLine(0.0f, 5.0f);

            std::string visible;
            if (i == 0 && !root.empty())
            {
#if defined(_WIN32)
                visible = root.string();
#else
                visible = "/";
#endif
            }
            else
            {
                visible = crumbs[i].filename().string();
                if (visible.empty()) visible = "/";
            }

            std::string id = visible + "##crumb_" + std::to_string(i);

            if (ImGui::SmallButton(id.c_str()))
            {
                m_CurrentDirectory = crumbs[i];
                std::snprintf(m_PathBuffer, sizeof(m_PathBuffer), "%s", m_CurrentDirectory.string().c_str());
            }

            if (i < crumbs.size() - 1)
            {
                ImGui::SameLine();
                ImGui::TextUnformatted(">");
                ImGui::SameLine();
            }
        }

        static float backAlpha = 1.0f;
        if (m_CurrentDirectory != std::filesystem::path(m_BaseDirectory))
        {
            ImGui::SameLine();
            ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.3f, 0.5f, 1.0f, backAlpha));
            if (ImGui::Button("<- Back"))
            {
                m_CurrentDirectory = m_CurrentDirectory.parent_path();
                backAlpha = 0.5f;
                std::snprintf(m_PathBuffer, sizeof(m_PathBuffer), "%s", m_CurrentDirectory.string().c_str());
            }
            ImGui::PopStyleColor();
            backAlpha += 0.05f; if (backAlpha > 1.0f) backAlpha = 1.0f;
        }
    }

    void ContentBrowser::DrawToolbar()
    {
        ImGui::Checkbox("Group by Type", &m_GroupByType);

        ImGui::SameLine();
        if (ImGui::Button(m_ListView ? "Grid" : "List")) m_ListView = !m_ListView;

        ImGui::SameLine();
        ImGui::SliderFloat("Thumb", &m_ThumbSize, 48.0f, 192.0f, "%.0f");

        ImGui::SameLine();
        if (ImGui::Button("Refresh")) {
            
        }

        ImGui::SameLine();
        ImGui::Checkbox("Show hidden", &m_ShowHidden);

        ImGui::SameLine();
        ImGui::Checkbox("Watch", &m_WatchEnabled);

        //ImGui::SameLine();
        ImGui::InputTextWithHint("##search", "Search files...", m_SearchBuffer, IM_ARRAYSIZE(m_SearchBuffer));
        std::string searchFilter = m_SearchBuffer;

        ImGui::SameLine();

        const char* sortLabel =
            m_SortMode == SortMode::NameAsc ? "Name Up" :
            m_SortMode == SortMode::NameDesc ? "Name Down" :
            m_SortMode == SortMode::DateAsc ? "Date Up" : "Date Down";

        if (ImGui::BeginCombo("Sort By", sortLabel)) {
            if (ImGui::Selectable("Name Up"))   m_SortMode = SortMode::NameAsc;
            if (ImGui::Selectable("Name Down")) m_SortMode = SortMode::NameDesc;
            if (ImGui::Selectable("Date Up"))   m_SortMode = SortMode::DateAsc;
            if (ImGui::Selectable("Date Down")) m_SortMode = SortMode::DateDesc;
            ImGui::EndCombo();
        }

        const char* names[] = { "All","Textures","Meshes","Scripts","Scenes","Other" };

        ImGui::PushID("FilterBar");
        for (int i = 0; i < 6; ++i) {
            if (i == 0) ImGui::Separator();
            if (i) ImGui::SameLine();

            std::string label = std::string(names[i]) + "##filter_" + std::to_string(i);
            if (ImGui::RadioButton(label.c_str(), m_TypeFilter == i))
                m_TypeFilter = i;
        }
        ImGui::PopID();

        if (ImGui::InputText("##path", m_PathBuffer, IM_ARRAYSIZE(m_PathBuffer), ImGuiInputTextFlags_AutoSelectAll)) {
            
        }
        ImGui::SameLine();
        if (ImGui::Button("Go")) {
            std::filesystem::path np(m_PathBuffer);
            if (std::filesystem::exists(np) && std::filesystem::is_directory(np)) {
                m_CurrentDirectory = np;
            }
            std::snprintf(m_PathBuffer, sizeof(m_PathBuffer), "%s", m_CurrentDirectory.string().c_str());
        }

        ImGui::SameLine();
        bool canPaste = m_Clipboard.mode != ClipMode::None && !m_Clipboard.items.empty();
        ImGui::BeginDisabled(!canPaste);
        if (ImGui::Button("Paste")) {
            std::error_code ec;
            for (auto& src : m_Clipboard.items) {
                auto dst = m_CurrentDirectory / src.filename();
                if (m_Clipboard.mode == ClipMode::Copy) { ec.clear(); CopyRec(src, dst, ec); }
                else if (m_Clipboard.mode == ClipMode::Cut) { ec.clear(); std::filesystem::rename(src, dst, ec); }
            }
            if (m_Clipboard.mode == ClipMode::Cut) { m_Clipboard = {}; }
        }
        ImGui::EndDisabled();
    }

    void ContentBrowser::DrawListHeaderIfNeeded()
    {
        if (m_ListView) {
            
        }
    }

    void ContentBrowser::DrawPreviewPane(float rightPaneWidth)
    {
        if (rightPaneWidth < 200.0f || m_Selection.empty()) return;

        ImGui::SameLine();
        ImGui::BeginChild("##preview", ImVec2(rightPaneWidth, 0), true);

        auto first = *m_Selection.begin();
        std::filesystem::path p(first);
        ImGui::Text("Preview");
        ImGui::Separator();
        ImGui::TextWrapped("%s", p.filename().string().c_str());
        std::error_code ec;
        if (std::filesystem::is_directory(p)) {
            ImGui::Text("Folder");
            ImGui::Text("Size: %s", PrettySize(DirSize(p)).c_str());
        }
        else {
            auto sz = std::filesystem::file_size(p, ec);
            ImGui::Text("File");
            ImGui::Text("Size: %s", PrettySize((uint64_t)sz).c_str());
            auto ext = p.extension().string();
            ImGui::Text("Ext: %s", ext.c_str());
        }
        auto tp = std::filesystem::last_write_time(p, ec);
        if (!ec) {
            auto tt = ToTimeT(tp);
            ImGui::Text("Modified: %s", std::asctime(std::localtime(&tt)));
        }

        ImGui::EndChild();
    }

    void ContentBrowser::HandleKeyboardShortcuts(bool windowFocused)
    {
        if (!windowFocused) return;

        if (ImGui::IsKeyPressed(ImGuiKey_Backspace) && m_CurrentDirectory != m_BaseDirectory) {
            m_CurrentDirectory = m_CurrentDirectory.parent_path();
            std::snprintf(m_PathBuffer, sizeof(m_PathBuffer), "%s", m_CurrentDirectory.string().c_str());
        }

        if (ImGui::IsKeyDown(ImGuiMod_Ctrl) && ImGui::IsKeyPressed(ImGuiKey_C)) {
            if (!m_Selection.empty()) {
                m_Clipboard.mode = ClipMode::Copy;
                m_Clipboard.items.clear();
                for (auto& s : m_Selection) m_Clipboard.items.push_back(std::filesystem::path(s));
            }
        }
        if (ImGui::IsKeyDown(ImGuiMod_Ctrl) && ImGui::IsKeyPressed(ImGuiKey_X)) {
            if (!m_Selection.empty()) {
                m_Clipboard.mode = ClipMode::Cut;
                m_Clipboard.items.clear();
                for (auto& s : m_Selection) m_Clipboard.items.push_back(std::filesystem::path(s));
            }
        }
        if (ImGui::IsKeyDown(ImGuiMod_Ctrl) && ImGui::IsKeyPressed(ImGuiKey_V)) {
            std::error_code ec;
            for (auto& src : m_Clipboard.items) {
                auto dst = m_CurrentDirectory / src.filename();
                if (m_Clipboard.mode == ClipMode::Copy) { ec.clear(); CopyRec(src, dst, ec); }
                else if (m_Clipboard.mode == ClipMode::Cut) { ec.clear(); std::filesystem::rename(src, dst, ec); }
            }
            if (m_Clipboard.mode == ClipMode::Cut) m_Clipboard = {};
        }

        if (ImGui::IsKeyPressed(ImGuiKey_Delete)) {
            m_PendingDelete.clear();
            for (auto& s : m_Selection) m_PendingDelete.push_back(std::filesystem::path(s));
            if (!m_PendingDelete.empty()) m_ShowConfirmDelete = true;
        }

        if (ImGui::IsKeyPressed(ImGuiKey_F2)) {
            OpenRenameForSingleSelection();
        }

        if (ImGui::IsKeyPressed(ImGuiKey_Enter)) {
            if (!m_Selection.empty()) {
                auto first = std::filesystem::path(*m_Selection.begin());
                if (std::filesystem::is_directory(first)) {
                    m_CurrentDirectory = first;
                    std::snprintf(m_PathBuffer, sizeof(m_PathBuffer), "%s", m_CurrentDirectory.string().c_str());
                }
            }
        }
    }

    void ContentBrowser::OpenRenameForSingleSelection()
    {
        if (m_Selection.size() == 1) {
            auto p = std::filesystem::path(*m_Selection.begin());
            m_RenamingPath = p;
            std::string filename = p.filename().string();
            std::memset(m_RenameBuffer, 0, sizeof(m_RenameBuffer));
            std::strncpy(m_RenameBuffer, filename.c_str(), sizeof(m_RenameBuffer) - 1);
        }
    }

    void ContentBrowser::DrawFolderTreePanel()
    {
        ImGui::TextDisabled("Folders");
        ImGui::Separator();

        DrawFolderNode(m_BaseDirectory);
    }

    void ContentBrowser::DrawFolderNode(const std::filesystem::path& dir)
    {
        std::error_code ec;
        if (!std::filesystem::exists(dir, ec) || !std::filesystem::is_directory(dir, ec))
            return;

        std::string label = dir.filename().string();
        if (label.empty()) label = dir.string();

        ImGuiTreeNodeFlags flags = ImGuiTreeNodeFlags_OpenOnArrow
            | ImGuiTreeNodeFlags_SpanFullWidth
            | ImGuiTreeNodeFlags_FramePadding;

        if (dir == m_CurrentDirectory)
            flags |= ImGuiTreeNodeFlags_Selected;

        bool has_child = false;
        {
            for (auto it = std::filesystem::directory_iterator(dir, ec);
                !ec && it != std::filesystem::directory_iterator(); ++it)
            {
                if (it->is_directory(ec)) {
                    auto name = it->path().filename().string();
                    if (!m_ShowHidden && !name.empty() && name[0] == '.') continue;
                    has_child = true; break;
                }
            }
        }
        if (!has_child) flags |= ImGuiTreeNodeFlags_Leaf;

        ImGui::PushID(dir.generic_string().c_str());
        bool open = ImGui::TreeNodeEx(label.c_str(), flags);

        if (ImGui::IsItemClicked(ImGuiMouseButton_Left)) {
            m_CurrentDirectory = dir;
            std::snprintf(m_PathBuffer, sizeof(m_PathBuffer), "%s", m_CurrentDirectory.string().c_str());
            m_Selection.clear();
        }

        if (ImGui::IsItemActive() && ImGui::IsMouseDragging(ImGuiMouseButton_Left)) {
            if (ImGui::BeginDragDropSource()) {
#if defined(_WIN32)
                const wchar_t* w = dir.c_str();
                ImGui::SetDragDropPayload("CONTENT_BROWSER_ITEM", w, (wcslen(w) + 1) * sizeof(wchar_t));
#else
                std::string pUtf8 = dir.string();
                ImGui::SetDragDropPayload("CONTENT_BROWSER_ITEM", pUtf8.c_str(), pUtf8.size() + 1);
#endif
                ImGui::TextUnformatted(label.c_str());
                ImGui::EndDragDropSource();
            }
        }

        if (ImGui::BeginDragDropTarget()) {
            if (const ImGuiPayload* payload = ImGui::AcceptDragDropPayload("CONTENT_BROWSER_ITEM")) {
#if defined(_WIN32)
                const wchar_t* wpath = (const wchar_t*)payload->Data;
                std::filesystem::path src(wpath);
#else
                const char* cpath = (const char*)payload->Data;
                std::filesystem::path src(cpath);
#endif
                std::error_code mec;
                if (!src.empty() && src != dir) {
                    auto src_abs = std::filesystem::weakly_canonical(src, mec);
                    auto dst_abs = std::filesystem::weakly_canonical(dir, mec);
                    bool into_self = false;
                    if (!mec) {
                        auto s = src_abs.generic_string() + "/";
                        auto d = dst_abs.generic_string() + "/";
                        if (d.find(s) == 0) into_self = true;
                    }
                    if (!into_self) {
                        auto dst = dir / src.filename();
                        mec.clear();
                        std::filesystem::rename(src, dst, mec);
                        if (mec) m_LastError = mec.message();
                    }
                }
            }
            ImGui::EndDragDropTarget();
        }

        if (open)
        {
            std::vector<std::filesystem::path> subdirs;
            for (auto it = std::filesystem::directory_iterator(dir, ec);
                !ec && it != std::filesystem::directory_iterator(); ++it)
            {
                if (!it->is_directory(ec)) continue;
                auto name = it->path().filename().string();
                if (!m_ShowHidden && !name.empty() && name[0] == '.') continue;
                subdirs.push_back(it->path());
            }
            std::sort(subdirs.begin(), subdirs.end(),
                [](const std::filesystem::path& a, const std::filesystem::path& b) {
                    return a.filename().string() < b.filename().string();
                });

            for (auto& sd : subdirs)
                DrawFolderNode(sd);

            ImGui::TreePop();
        }

        ImGui::PopID();
    }

    bool ContentBrowser::PassesSearchFilter(const std::string& filename, const std::string& filter) const
    {
        if (filter.empty()) return true;
        auto it = std::search(
            filename.begin(), filename.end(),
            filter.begin(), filter.end(),
            [](char a, char b) { return std::tolower((unsigned char)a) == std::tolower((unsigned char)b); }
        );
        return it != filename.end();
    }

    bool ContentBrowser::PassesTypeFilter(int typeFilter, int assetTypeValue) const
    {
        if (typeFilter == 0) return true;
        if (typeFilter == 1) return assetTypeValue == (int)AssetType::TEXTURE;
        if (typeFilter == 2) return assetTypeValue == (int)AssetType::MESH;
        if (typeFilter == 3) return assetTypeValue == (int)AssetType::SCRIPT;
        if (typeFilter == 4) return assetTypeValue == (int)AssetType::SCENE;
        if (typeFilter == 5) {
            return !(assetTypeValue == (int)AssetType::TEXTURE ||
                assetTypeValue == (int)AssetType::MESH ||
                assetTypeValue == (int)AssetType::SCRIPT ||
                assetTypeValue == (int)AssetType::SCENE);
        }
        return true;
    }

    void ContentBrowser::RenderUI()
    {
        if (m_TextureViewer) {
            if (!m_TextureViewer->OpenFlag()) m_TextureViewer.reset();
            else m_TextureViewer->RenderUI();
        }
        if (m_CodeEditor) m_CodeEditor->RenderUI();

        ImGui::Begin("Content Browser");

        const float min_left = 160.0f;
        const float max_left = std::max(160.0f, ImGui::GetContentRegionAvail().x - 300.0f);

        ImGui::BeginChild("##left_tree_panel", ImVec2(m_LeftPaneWidth, 0), true, ImGuiWindowFlags_NoMove);
        DrawFolderTreePanel();
        ImGui::EndChild();

        ImGui::SameLine(0.0f, 0.0f);
        ImGui::InvisibleButton("##vsplitter", ImVec2(m_SplitterWidth, -1));
        if (ImGui::IsItemActive())
            m_LeftPaneWidth = std::clamp(m_LeftPaneWidth + ImGui::GetIO().MouseDelta.x, min_left, max_left);
        if (ImGui::IsItemHovered())
            ImGui::SetMouseCursor(ImGuiMouseCursor_ResizeEW);

        ImGui::SameLine(0.0f, 0.0f);
        ImGui::BeginChild("##right_content", ImVec2(0, 0), false);

        if (ImGui::IsWindowHovered(ImGuiHoveredFlags_AllowWhenBlockedByActiveItem) &&
            ImGui::IsMouseClicked(ImGuiMouseButton_Left) &&
            !ImGui::IsAnyItemHovered())
        {
            m_Selection.clear();
        }

        if (ImGui::BeginDragDropTarget()) {
            if (const ImGuiPayload* payload = ImGui::AcceptDragDropPayload("EXTERNAL_FILE")) {
            }
            ImGui::EndDragDropTarget();
        }

        if (!m_LastError.empty()) {
            ImGui::TextColored(ImVec4(1, 0.4f, 0.4f, 1), "Erreur: %s", m_LastError.c_str());
        }

        DrawBreadcrumbs();
        DrawToolbar();
        DrawListHeaderIfNeeded();

        std::vector<std::filesystem::directory_entry> entries;
        std::error_code ecIter;
        for (auto& dirEntry : std::filesystem::directory_iterator(m_CurrentDirectory, ecIter)) {
            if (ecIter) break;
            std::string filenameStr = dirEntry.path().filename().string();
            if (!m_ShowHidden && !filenameStr.empty() && filenameStr[0] == '.') continue;
            std::string ext = dirEntry.path().extension().string();
            bool ignore = false;
            for (auto& ign : m_IgnoredExtensions) if (ext == ign) { ignore = true; break; }
            if (ignore) continue;
            if (!PassesSearchFilter(filenameStr, m_SearchBuffer)) continue;
            entries.push_back(dirEntry);
        }

        auto BeginDragSourceForPath = [&](const std::filesystem::path& p, const std::string& preview)
            {
                if (ImGui::BeginDragDropSource(ImGuiDragDropFlags_SourceAllowNullID))
                {
#if defined(_WIN32)
                    const wchar_t* w = p.c_str();
                    ImGui::SetDragDropPayload("CONTENT_BROWSER_ITEM", w, (wcslen(w) + 1) * sizeof(wchar_t));
#else
                    std::string u8 = p.string();
                    ImGui::SetDragDropPayload("CONTENT_BROWSER_ITEM", u8.c_str(), u8.size() + 1);
#endif
                    ImGui::TextUnformatted(preview.c_str());
                    ImGui::EndDragDropSource();
                }
            };

        auto compareFunc = [&](const std::filesystem::directory_entry& a, const std::filesystem::directory_entry& b)
            {
                switch (m_SortMode) {
                case SortMode::NameAsc:  return a.path().filename().string() < b.path().filename().string();
                case SortMode::NameDesc: return a.path().filename().string() > b.path().filename().string();
                case SortMode::DateAsc:  return std::filesystem::last_write_time(a) < std::filesystem::last_write_time(b);
                case SortMode::DateDesc: return std::filesystem::last_write_time(a) > std::filesystem::last_write_time(b);
                }
                return false;
            };
        std::sort(entries.begin(), entries.end(), compareFunc);

        float cardPadding = 12.0f;
        float cardInnerPad = 8.0f;
        float textLines = 2.0f;
        float textH = ImGui::GetTextLineHeightWithSpacing() * textLines;
        float iconSize = m_ThumbSize;
        float cardW = iconSize + cardPadding * 2.0f;
        float cardH_base = iconSize + textH + cardPadding * 2.0f + 6.0f;
        float extraVGap = 12.0f;

        float panelWidth = ImGui::GetContentRegionAvail().x;
        float rightPaneW = m_ListView ? 0.0f : std::min(280.0f, panelWidth * 0.35f);
        if (rightPaneW > 0.0f) panelWidth -= rightPaneW + ImGui::GetStyle().ItemSpacing.x;

        int columnCount = m_ListView ? 1 : std::max(1, (int)std::floor(panelWidth / (cardW + ImGui::GetStyle().ItemSpacing.x)));
        columnCount = std::max(columnCount, 1);

        static float s_meshPanelHeight = 140.0f;

        auto getAssetIconAndId = [&](const std::filesystem::path& path, AssetType& fileType, std::shared_ptr<Texture2D>& icon, std::string& modelId, bool& isModel)
            {
                std::string extension = path.extension().string();
                fileType = AssetManager::Instance().getTypeFromExtention(extension);
                isModel = false;
                if (std::filesystem::is_directory(path)) {
                    icon = m_DirectoryIcon;
                    return;
                }
                if (fileType == AssetType::MODEL) {
                    icon = m_FileOBJIcon;
                    std::error_code ec{};
                    if (auto rel = std::filesystem::relative(path, m_BaseDirectory, ec); !ec)
                        modelId = "Assets/" + rel.generic_string();
                    else
                        modelId = "Assets/" + path.filename().generic_string();
                    isModel = true;

                    /*if (!AssetManager::Instance().isAssetLoaded(modelId)) {
                        static std::unordered_map<std::string, bool> loadingModel;
                        if (!loadingModel[modelId]) {
                            loadingModel[modelId] = true;
                            ModelImportOptions opts;
                            opts.buildMeshes = false;
                            opts.loadMaterials = false;
                            opts.loadSkinning = false;
                            opts.loadAnimations = false;
                            opts.vertexLayout.reset();
                            AssetToLoad a{};
                            a.id = modelId;
                            a.path = path.generic_string();
                            a.type = AssetType::MODEL;
                            a.spec = opts;
                            AssetManager::Instance().loadAsset(a);
                        }
                    }*/
                    return;
                }
                if (fileType == AssetType::TEXTURE) {
                    const std::filesystem::path texAbs = path;
                    std::error_code ec{};
                    std::string id;
                    if (auto rel = std::filesystem::relative(texAbs, m_BaseDirectory, ec); !ec)
                        id = "Assets/" + rel.generic_string();
                    else
                        id = "Assets/" + texAbs.filename().generic_string();

                    if (AssetManager::Instance().isAssetLoaded(id)) {
                        icon = AssetManager::Instance().getAsset<Texture2D>(id);
                    }
                    else {
                        static std::unordered_map<std::string, bool> loadingMap;
                        if (!loadingMap[id]) {
                            loadingMap[id] = true;
                            TextureSpecification spec = TextureConfigImporter::ImportTextureConfig(texAbs.generic_string());
                            AssetToLoad asset{};
                            asset.id = id;
                            asset.path = texAbs.generic_string();
                            asset.type = AssetType::TEXTURE;
                            asset.spec = spec;
                            AssetManager::Instance().loadAsset(asset);
                        }
                        icon = AssetManager::Instance().isAssetLoaded(id) ? AssetManager::Instance().getAsset<Texture2D>(id) : m_FileOtherIcon;
                    }
                    return;
                }
                if (fileType == AssetType::SCRIPT) {
                    std::string ext = path.extension().string();
                    icon = (ext == ".lua") ? m_FileLuaIcon : m_FileOtherIcon;
                    return;
                }
                if (fileType == AssetType::SCENE) {
                    icon = m_FileSceneIcon;
                    return;
                }
                icon = m_FileOtherIcon;
            };

        auto drawMeshesPanel = [&](const std::string& itemPath, const std::string& modelId, float width)
            {
                ImGui::PushStyleColor(ImGuiCol_ChildBg, ImVec4(1, 1, 1, 0.03f));
                ImGui::BeginChild((std::string("##mesh_panel_") + itemPath).c_str(), ImVec2(width, s_meshPanelHeight), true);
                ImGui::SetCursorPosY(ImGui::GetCursorPosY() + 2.0f);
                ImGui::SliderFloat("Height", &s_meshPanelHeight, 80.0f, 280.0f, "%.0f");
                ImGui::Separator();

                auto model = AssetManager::Instance().getAsset<Model>(modelId);
                if (model) {
                    std::unordered_map<std::string, std::vector<std::string>> byNode;
                    model->ForEachInstance([&](const MeshInstance& inst, const glm::mat4&, const std::string& nodePath) {
                        byNode[nodePath].push_back(inst.name);
                        });
                    for (auto& kv : byNode) {
                        ImGui::TextDisabled("%s", kv.first.c_str());
                        ImGui::Indent();
                        for (auto& n : kv.second) {
                            ImGui::PushID((itemPath + "_mesh_" + n).c_str());
                            if (ImGui::Selectable(n.c_str(), false, ImGuiSelectableFlags_AllowDoubleClick, ImVec2(0, 0))) {
                            }
                            if (ImGui::BeginDragDropSource(ImGuiDragDropFlags_SourceAllowNullID)) {
                                ImGui::SetDragDropPayload("CONTENT_BROWSER_MESH_NAME", n.c_str(), (n.size() + 1));
                                ImGui::TextUnformatted(n.c_str());
                                ImGui::EndDragDropSource();
                            }
                            ImGui::PopID();
                        }
                        ImGui::Unindent();
                        ImGui::Dummy(ImVec2(0, 2));
                    }
                }
                else {
                    ImGui::TextDisabled("Loading...");
                }

                ImGui::EndChild();
                ImGui::PopStyleColor();
            };

        auto itemContextMenu = [&](const std::filesystem::path& path, const std::string& filenameString)
            {
                if (ImGui::MenuItem("Rename")) {
                    m_RenamingPath = path;
                    std::memset(m_RenameBuffer, 0, sizeof(m_RenameBuffer));
                    std::strncpy(m_RenameBuffer, filenameString.c_str(), sizeof(m_RenameBuffer) - 1);
                }
                if (ImGui::MenuItem("Copy")) {
                    m_Clipboard.mode = ClipMode::Copy;
                    m_Clipboard.items = { path };
                }
                if (ImGui::MenuItem("Cut")) {
                    m_Clipboard.mode = ClipMode::Cut;
                    m_Clipboard.items = { path };
                }
                if (ImGui::MenuItem("Paste")) {
                    if (m_Clipboard.mode != ClipMode::None && !m_Clipboard.items.empty()) {
                        std::error_code ec;
                        for (auto& src : m_Clipboard.items) {
                            auto dst = path.parent_path() / src.filename();
                            if (m_Clipboard.mode == ClipMode::Copy) { ec.clear(); CopyRec(src, dst, ec); }
                            else if (m_Clipboard.mode == ClipMode::Cut) { ec.clear(); std::filesystem::rename(src, dst, ec); }
                        }
                        if (m_Clipboard.mode == ClipMode::Cut) m_Clipboard = {};
                    }
                }
                if (ImGui::MenuItem("Duplicate")) {
                    std::error_code ecDup;
                    auto base = path.filename().string();
                    auto dup = UniqueNameInDir(path.parent_path(), base + " - Copy");
                    ecDup.clear(); CopyRec(path, dup, ecDup);
                }
                if (ImGui::MenuItem("Compress (.zip)")) {
                }
                if (ImGui::MenuItem("Extract here")) {
                }
                if (ImGui::MenuItem("Reveal in Explorer")) {
#if defined(_WIN32)
                    std::string cmd = "explorer /select,\"" + path.string() + "\"";
                    system(cmd.c_str());
#elif defined(__APPLE__)
                    std::string cmd = "open -R \"" + path.string() + "\"";
                    system(cmd.c_str());
#else
                    std::string cmd = "xdg-open \"" + path.parent_path().string() + "\"";
                    system(cmd.c_str());
#endif
                }
                ImGui::Separator();
                if (ImGui::MenuItem("Delete")) {
                    m_PendingDelete.clear();
                    m_PendingDelete.push_back(path);
                    m_ShowConfirmDelete = true;
                }
            };

        if (m_ListView)
        {
            ImGuiTableFlags tflags = ImGuiTableFlags_SizingStretchProp | ImGuiTableFlags_RowBg | ImGuiTableFlags_BordersOuter | ImGuiTableFlags_BordersInnerH;
            if (ImGui::BeginTable("##ContentList", 5, tflags))
            {
                ImGui::TableSetupColumn("Name", ImGuiTableColumnFlags_WidthStretch, 0.48f);
                ImGui::TableSetupColumn("Type", ImGuiTableColumnFlags_WidthStretch, 0.14f);
                ImGui::TableSetupColumn("Size", ImGuiTableColumnFlags_WidthStretch, 0.12f);
                ImGui::TableSetupColumn("Modified", ImGuiTableColumnFlags_WidthStretch, 0.22f);
                ImGui::TableSetupColumn("", ImGuiTableColumnFlags_WidthFixed, 24.0f);
                ImGui::TableHeadersRow();

                const float rowHeight = 38.0f;
                const float icon24 = 24.0f;

                for (auto& directoryEntry : entries)
                {
                    const auto& path = directoryEntry.path();
                    std::string filenameString = path.filename().string();
                    if (filenameString.empty()) filenameString = "unknown";
                    std::string itemPath = path.generic_string();

                    AssetType fileType; std::shared_ptr<Texture2D> icon; std::string modelId; bool isModel = false;
                    getAssetIconAndId(path, fileType, icon, modelId, isModel);
                    if (!PassesTypeFilter(m_TypeFilter, (int)fileType)) continue;

                    bool isSelected = (m_Selection.find(itemPath) != m_Selection.end());

                    ImGui::TableNextRow(ImGuiTableRowFlags_None, rowHeight);
                    ImGui::TableSetColumnIndex(0);

                    ImGuiSelectableFlags sflags = ImGuiSelectableFlags_SpanAllColumns | ImGuiSelectableFlags_AllowDoubleClick | ImGuiSelectableFlags_AllowItemOverlap;
                    if (ImGui::Selectable((std::string("##row_") + itemPath).c_str(), isSelected, sflags, ImVec2(0, rowHeight))) {
                        if (ImGui::GetIO().KeyCtrl) {
                            if (isSelected) m_Selection.erase(itemPath);
                            else m_Selection.insert(itemPath);
                        }
                        else {
                            m_Selection.clear();
                            m_Selection.insert(itemPath);
                        }
                    }

                    if (ImGui::BeginPopupContextItem((std::string("##ctx_row_full_") + itemPath).c_str())) {
                        itemContextMenu(path, filenameString);
                        ImGui::EndPopup();
                    }

                    ImVec2 startPos = ImGui::GetCursorPos();
                    ImGui::SetCursorPosY(startPos.y - rowHeight + (rowHeight - icon24) * 0.5f);
                    ImGui::Image((ImTextureID)icon->GetHandle(), ImVec2(icon24, icon24), ImVec2(0, 1), ImVec2(1, 0));
                    ImGui::SameLine();

                    if (m_RenamingPath && *m_RenamingPath == path) {
                        ImGui::SetNextItemWidth(std::max(180.0f, ImGui::GetContentRegionAvail().x * 0.3f));
                        if (ImGui::InputText((std::string("##rename_") + itemPath).c_str(), m_RenameBuffer, IM_ARRAYSIZE(m_RenameBuffer), ImGuiInputTextFlags_EnterReturnsTrue)) {
                            std::error_code ec;
                            auto newP = path.parent_path() / m_RenameBuffer;
                            std::filesystem::rename(path, newP, ec);
                            m_RenamingPath.reset();
                            m_Selection.clear();
                            m_Selection.insert(newP.generic_string());
                        }
                    }
                    else {
                        ImGui::TextUnformatted(filenameString.c_str());
                    }

                    ImGui::TableSetColumnIndex(1);
                    if (directoryEntry.is_directory()) ImGui::TextUnformatted("Folder");
                    else {
                        auto ext = path.extension().string();
                        ImGui::TextUnformatted(ext.empty() ? "-" : ext.c_str());
                    }

                    ImGui::TableSetColumnIndex(2);
                    if (directoryEntry.is_directory()) ImGui::TextUnformatted("-");
                    else {
                        std::error_code ecSz; auto sz = std::filesystem::file_size(path, ecSz);
                        ImGui::Text("%s", PrettySize((uint64_t)sz).c_str());
                    }

                    ImGui::TableSetColumnIndex(3);
                    {
                        std::error_code ecm; auto tp = std::filesystem::last_write_time(path, ecm);
                        if (!ecm) { auto tt = ToTimeT(tp); ImGui::Text("%s", std::asctime(std::localtime(&tt))); }
                        else ImGui::TextUnformatted("-");
                    }

                    ImGui::TableSetColumnIndex(4);
                    if (isModel) {
                        std::string toggleId = "##toggle_row_" + itemPath;
                        if (ImGui::SmallButton(toggleId.c_str())) {
                            m_ExpandedModels[itemPath] = !m_ExpandedModels[itemPath];
                        }
                    }

                    if (isModel && m_ExpandedModels[itemPath]) {
                        ImGui::TableNextRow();
                        ImGui::TableSetColumnIndex(0);
                        drawMeshesPanel(itemPath, modelId, ImGui::GetContentRegionAvail().x + ImGui::GetStyle().CellPadding.x * 2.0f);
                    }
                }

                ImGui::EndTable();
            }
        }
        else
        {
            ImGuiTableFlags tflags = ImGuiTableFlags_SizingFixedFit | ImGuiTableFlags_NoBordersInBody | ImGuiTableFlags_BordersOuterV;
            if (ImGui::BeginTable("##ContentGrid", columnCount, tflags))
            {
                ImGui::PushStyleVar(ImGuiStyleVar_CellPadding, ImVec2(10.0f, 12.0f));

                auto drawMeshCard = [&](const std::string& parentId, const std::string& meshName, const std::string& nodePath)
                    {
                        ImGui::TableNextColumn();

                        std::string mid = parentId + "##mesh_" + meshName;
                        ImGui::PushID(mid.c_str());

                        float meshPadding = 10.0f;
                        float meshIconSize = std::max(48.0f, iconSize * 0.6f);
                        float meshTextLines = 2.0f;
                        float meshTextH = ImGui::GetTextLineHeightWithSpacing() * meshTextLines;
                        float meshW = cardW * 0.9f;
                        float meshH = meshIconSize + meshTextH + meshPadding * 2.0f;

                        ImVec2 p0 = ImGui::GetCursorScreenPos();

                        ImGui::InvisibleButton("##mesh_hit", ImVec2(meshW, meshH));
                        bool hovered = ImGui::IsItemHovered();

                        ImDrawList* dl = ImGui::GetWindowDrawList();
                        float rounding = 10.0f;
                        ImU32 colShadow = ImGui::GetColorU32(ImVec4(0, 0, 0, 0.10f));
                        ImU32 colBg = ImGui::GetColorU32(hovered ? ImVec4(1, 1, 1, 0.08f) : ImVec4(1, 1, 1, 0.05f));
                        ImU32 colBorder = ImGui::GetColorU32(ImVec4(1, 1, 1, 0.12f));

                        ImVec2 cardMin = p0;
                        ImVec2 cardMax = ImVec2(p0.x + meshW, p0.y + meshH);

                        dl->AddRectFilled(cardMin + ImVec2(0, 1), cardMax + ImVec2(0, 1), colShadow, rounding);
                        dl->AddRectFilled(cardMin, cardMax, colBg, rounding);
                        dl->AddRect(cardMin, cardMax, colBorder, rounding);

                        ImVec2 iconPos(cardMin.x + (meshW - meshIconSize) * 0.5f, cardMin.y + meshPadding);
                        dl->AddImage((ImTextureID)m_FileOBJIcon->GetHandle(),
                            iconPos, iconPos + ImVec2(meshIconSize, meshIconSize),
                            ImVec2(0, 1), ImVec2(1, 0));

                        const float textAreaW = meshW - meshPadding * 2.0f;
                        ImVec2 textTop(cardMin.x + (meshW - textAreaW) * 0.5f,
                            iconPos.y + meshIconSize + 6.0f);

                        auto drawCenteredWrap = [&](const std::string& txt)
                            {
                                const float line_h = ImGui::GetTextLineHeightWithSpacing();
                                const float space_w = ImGui::CalcTextSize(" ").x;
                                const char* cur = txt.c_str();
                                const char* end = cur + txt.size();
                                ImVec2 pos = textTop;

                                for (int line = 0; line < 2 && cur < end; ++line) {
                                    const char* line_start = cur;
                                    const char* draw_end = cur;
                                    float line_w = 0.0f;

                                    while (cur < end) {
                                        while (cur < end && (*cur == ' ' || *cur == '\t' || *cur == '\n' || *cur == '\r')) {
                                            if (*cur == '\n') break; ++cur;
                                        }
                                        if (cur >= end || *cur == '\n') { ++cur; break; }

                                        const char* w_end = cur;
                                        while (w_end < end && *w_end != ' ' && *w_end != '\t' && *w_end != '\n' && *w_end != '\r') ++w_end;

                                        ImVec2 w_size = ImGui::CalcTextSize(cur, w_end);
                                        float next_w = (draw_end == line_start) ? w_size.x : (line_w + space_w + w_size.x);
                                        if (next_w > textAreaW && draw_end != line_start) break;

                                        line_w = (draw_end == line_start) ? w_size.x : (line_w + space_w + w_size.x);
                                        draw_end = w_end;
                                        cur = w_end;
                                    }

                                    float x = cardMin.x + (meshW - line_w) * 0.5f;
                                    dl->AddText(ImVec2(x, pos.y), ImGui::GetColorU32(ImVec4(1, 1, 1, 0.95f)), line_start, draw_end);
                                    pos.y += line_h;
                                }
                            };

                        drawCenteredWrap(meshName);

                        if (ImGui::BeginDragDropSource(ImGuiDragDropFlags_SourceAllowNullID)) {
                            ImGui::SetDragDropPayload("CONTENT_BROWSER_MESH_NAME", meshName.c_str(), (meshName.size() + 1));
                            ImGui::TextUnformatted(meshName.c_str());
                            ImGui::EndDragDropSource();
                        }

                        ImGui::Dummy(ImVec2(0, 8.0f));

                        ImGui::PopID();
                    };

                for (auto& directoryEntry : entries)
                {
                    const auto& path = directoryEntry.path();
                    std::string filenameString = path.filename().string();
                    if (filenameString.empty()) filenameString = "unknown";
                    std::string itemPath = path.generic_string();

                    AssetType fileType;
                    std::shared_ptr<Texture2D> icon;
                    std::string modelId;
                    bool isModel = false;
                    getAssetIconAndId(path, fileType, icon, modelId, isModel);
                    if (!PassesTypeFilter(m_TypeFilter, (int)fileType)) continue;

                    ImGui::TableNextColumn();
                    ImGui::PushID(itemPath.c_str());

                    ImVec2 p0 = ImGui::GetCursorScreenPos();

                    float cardW_local = cardW;
                    float cardH_local = cardH_base;

                    bool expanded = m_ExpandedModels[itemPath];

                    ImGui::InvisibleButton("##card_hit", ImVec2(cardW_local, cardH_local));
                    ImGui::SetItemAllowOverlap();
                    bool cardIsActiveAndDragging = ImGui::IsItemActive() && ImGui::IsMouseDragging(ImGuiMouseButton_Left);
                    if (cardIsActiveAndDragging)
                    {
                        BeginDragSourceForPath(itemPath, filenameString);
                    }

                    bool hoveredCard = ImGui::IsItemHovered();
                    bool leftClicked = ImGui::IsItemClicked(ImGuiMouseButton_Left);
                    bool rightClicked = ImGui::IsItemClicked(ImGuiMouseButton_Right);
                    bool doubleClicked = hoveredCard && ImGui::IsMouseDoubleClicked(ImGuiMouseButton_Left);

                    if (ImGui::BeginPopupContextItem())
                    {
                        itemContextMenu(path, filenameString);
                        ImGui::EndPopup();
                    }

                    ImDrawList* dl = ImGui::GetWindowDrawList();
                    float rounding = 10.0f;
                    ImVec2 cardMin = p0;
                    ImVec2 cardMax = ImVec2(p0.x + cardW_local, p0.y + cardH_local);

                    ImU32 colShadow = ImGui::GetColorU32(ImVec4(0, 0, 0, 0.10f));
                    ImU32 colBg = ImGui::GetColorU32(hoveredCard ? ImVec4(1, 1, 1, 0.06f) : ImVec4(1, 1, 1, 0.03f));
                    ImU32 colBorder = ImGui::GetColorU32(ImVec4(1, 1, 1, 0.10f));
                    dl->AddRectFilled(cardMin + ImVec2(0, 1), cardMax + ImVec2(0, 1), colShadow, rounding);
                    dl->AddRectFilled(cardMin, cardMax, colBg, rounding);
                    dl->AddRect(cardMin, cardMax, colBorder, rounding);

                    ImVec2 iconPos(cardMin.x + cardPadding, cardMin.y + cardPadding);
                    dl->AddImage((ImTextureID)icon->GetHandle(),
                        iconPos, iconPos + ImVec2(iconSize, iconSize),
                        ImVec2(0, 1), ImVec2(1, 0));

                    const float textAreaW = cardW_local - cardPadding * 2.0f;
                    ImVec2 textTop(cardMin.x + (cardW_local - textAreaW) * 0.5f,
                        iconPos.y + iconSize + 6.0f);

                    size_t lastDot = filenameString.find_last_of('.');
                    std::string fileNameOnly = (lastDot == std::string::npos)
                        ? filenameString
                        : filenameString.substr(0, lastDot);

                    auto drawCenteredWrap = [&](const std::string& txt)
                        {
                            const float line_h = ImGui::GetTextLineHeightWithSpacing();
                            const float space_w = ImGui::CalcTextSize(" ").x;
                            const char* cur = txt.c_str();
                            const char* end = cur + txt.size();
                            ImVec2 pos = textTop;

                            for (int line = 0; line < 2 && cur < end; ++line) {
                                const char* line_start = cur;
                                const char* draw_end = cur;
                                float line_w = 0.0f;

                                while (cur < end) {
                                    while (cur < end && (*cur == ' ' || *cur == '\t' || *cur == '\n' || *cur == '\r')) {
                                        if (*cur == '\n') break; ++cur;
                                    }
                                    if (cur >= end || *cur == '\n') { ++cur; break; }
                                    const char* w_end = cur;
                                    while (w_end < end && *w_end != ' ' && *w_end != '\t' && *w_end != '\n' && *w_end != '\r') ++w_end;

                                    ImVec2 w_size = ImGui::CalcTextSize(cur, w_end);
                                    float next_w = (draw_end == line_start) ? w_size.x : (line_w + space_w + w_size.x);
                                    if (next_w > textAreaW && draw_end != line_start) break;
                                    line_w = (draw_end == line_start) ? w_size.x : (line_w + space_w + w_size.x);
                                    draw_end = w_end;
                                    cur = w_end;
                                }

                                float x = cardMin.x + (cardW_local - line_w) * 0.5f;
                                dl->AddText(ImVec2(x, pos.y), ImGui::GetColorU32(ImVec4(1, 1, 1, 0.95f)), line_start, draw_end);
                                pos.y += line_h;
                            }
                        };
                    drawCenteredWrap(fileNameOnly);

                    ImRect toggleRect;
                    bool toggleClicked = false;
                    if (isModel)
                    {
                        ImVec2 toggleSize(24.0f, 24.0f);
                        ImVec2 togglePos(cardMax.x - toggleSize.x - 6.0f, cardMin.y + 6.0f);
                        toggleRect = ImRect(togglePos, ImVec2(togglePos.x + toggleSize.x, togglePos.y + toggleSize.y));

                        ImGui::SetCursorScreenPos(togglePos);
                        ImGui::PushID("toggle");
                        ImGui::InvisibleButton("##toggle_btn", toggleSize);
                        bool tHovered = ImGui::IsItemHovered();
                        bool tClicked = ImGui::IsItemClicked();
                        ImGui::PopID();

                        const char* g = expanded ? "-" : "+";
                        ImVec2 gsz = ImGui::CalcTextSize(g);
                        ImU32 gcol = ImGui::GetColorU32(ImVec4(1, 1, 1, tHovered ? 0.95f : 0.80f));
                        dl->AddText(ImVec2(togglePos.x + (toggleSize.x - gsz.x) * 0.5f,
                            togglePos.y + (toggleSize.y - gsz.y) * 0.5f), gcol, g);

                        if (tClicked) toggleClicked = true;
                    }

                    if (toggleClicked) {
                        m_ExpandedModels[itemPath] = !m_ExpandedModels[itemPath];
                        expanded = m_ExpandedModels[itemPath];
                    }

                    if (leftClicked && !(isModel && toggleRect.Contains(ImGui::GetMousePos())))
                    {
                        bool isSelected = (m_Selection.find(itemPath) != m_Selection.end());
                        if (ImGui::GetIO().KeyCtrl) {
                            if (isSelected) m_Selection.erase(itemPath);
                            else m_Selection.insert(itemPath);
                        }
                        else {
                            m_Selection.clear();
                            m_Selection.insert(itemPath);
                        }
                    }

                    if (doubleClicked && !(isModel && toggleRect.Contains(ImGui::GetMousePos())))
                    {
                        auto ext = path.extension().string();
                        if (std::filesystem::is_directory(path)) {
                            m_CurrentDirectory /= path.filename();
                            std::snprintf(m_PathBuffer, sizeof(m_PathBuffer), "%s", m_CurrentDirectory.string().c_str());
                        }
                        else if (fileType == AssetType::TEXTURE) {
                            m_TextureViewer = std::make_shared<TextureViewer>(m_Context);
							m_TextureViewer->SetTexturePath(WeakCanonical(path));
                        }
                        else if (fileType == AssetType::SCRIPT) {
                            m_CodeEditor = std::make_shared<CodeEditor>(m_Context);
                            if (ext == ".lua") m_CodeEditor->SetLanguageDefinition(TextEditor::LanguageDefinition::Lua());
                            else if (ext == ".cpp" || ext == ".h") m_CodeEditor->SetLanguageDefinition(TextEditor::LanguageDefinition::CPlusPlus());
                            m_CodeEditor->LoadFromFile(path.string());
                        }
                        else {
                            m_CodeEditor = std::make_shared<CodeEditor>(m_Context);
                            if (ext == ".txt") m_CodeEditor->SetLanguageDefinition(TextEditor::LanguageDefinition::LanguageDefinition());
                            m_CodeEditor->LoadFromFile(path.string());
                        }
                    }

                    if (ImGui::BeginDragDropSource(ImGuiDragDropFlags_SourceAllowNullID)) {
#if defined(_WIN32)
                        const wchar_t* itemPathW = path.c_str();
                        ImGui::SetDragDropPayload("CONTENT_BROWSER_ITEM", itemPathW, (wcslen(itemPathW) + 1) * sizeof(wchar_t));
#else
                        std::string pUtf8 = path.string();
                        ImGui::SetDragDropPayload("CONTENT_BROWSER_ITEM", pUtf8.c_str(), pUtf8.size() + 1);
#endif
                        ImGui::TextUnformatted(filenameString.c_str());
                        ImGui::EndDragDropSource();
                    }

                    if (isModel && expanded)
                    {
                        auto model = AssetManager::Instance().getAsset<Model>(modelId);
                        if (model)
                        {
                            std::vector<std::pair<std::string, std::string>> meshCards;
                            model->ForEachInstance([&](const MeshInstance& inst, const glm::mat4&, const std::string& nodePath) {
                                meshCards.emplace_back(inst.name, nodePath);
                                });

                            int colIndex = ImGui::TableGetColumnIndex();
                            int remainingCols = columnCount - (colIndex + 1);

                            int idx = 0;
                            auto emitMeshCard = [&](const std::pair<std::string, std::string>& m) {
                                drawMeshCard(itemPath, m.first, m.second);
                                };

                            for (; idx < (int)meshCards.size(); ++idx)
                            {
                                if (remainingCols <= 0) {
                                    ImGui::TableNextRow();
                                    remainingCols = columnCount;
                                }
                                emitMeshCard(meshCards[idx]);
                                --remainingCols;
                            }

                            ImGui::TableNextRow();
                        }
                        else
                        {
                            ImGui::TableNextColumn();
                            ImGui::TextDisabled("Loading...");
                            ImGui::TableNextRow();
                        }
                    }
                    else
                    {
                        ImGui::SetCursorScreenPos(ImVec2(p0.x, p0.y + cardH_local + extraVGap));
                        ImGui::Dummy(ImVec2(cardW_local, 0.0f));
                    }

                    ImGui::PopID();
                }

                ImGui::PopStyleVar();
                ImGui::EndTable();
            }
        }

        if (m_ShowConfirmDelete) {
            ImGui::OpenPopup("Confirmer la suppression");
            m_ShowConfirmDelete = false;
        }
        if (ImGui::BeginPopupModal("Confirmer la suppression", nullptr, ImGuiWindowFlags_AlwaysAutoResize)) {
            ImGui::Text("Deplacer vers la Corbeille :");
            for (auto& p : m_PendingDelete) ImGui::BulletText("%s", p.filename().string().c_str());

            if (ImGui::Button("Confirmer", ImVec2(120, 0))) {
                m_LastError.clear();
                for (auto& p : m_PendingDelete) {
                    std::string err;
                    if (!MoveToTrash(p, m_TrashDir, err) && m_LastError.empty()) m_LastError = err;
                }
                for (auto& p : m_PendingDelete) m_Selection.erase(p.generic_string());
                ImGui::CloseCurrentPopup();
            }
            ImGui::SameLine();
            if (ImGui::Button("Annuler", ImVec2(120, 0))) ImGui::CloseCurrentPopup();
            ImGui::EndPopup();
        }

        if (ImGui::BeginPopupContextWindow("ContentBrowserContext",
            ImGuiPopupFlags_MouseButtonRight | ImGuiPopupFlags_NoOpenOverItems))
        {
            if (ImGui::MenuItem("New Folder")) {
                std::filesystem::path newFolder = m_CurrentDirectory / "New Folder";
                int counter = 1;
                while (std::filesystem::exists(newFolder)) {
                    newFolder = m_CurrentDirectory / ("New Folder " + std::to_string(counter++));
                }
                std::filesystem::create_directory(newFolder);
            }

            if (ImGui::BeginMenu("New File")) {
                if (ImGui::MenuItem("Text File")) {
                    std::filesystem::path nf = m_CurrentDirectory / "NewFile.txt";
                    int c = 1; while (std::filesystem::exists(nf)) nf = m_CurrentDirectory / ("NewFile" + std::to_string(c++) + ".txt");
                    std::ofstream ofs(nf); ofs << ""; ofs.close();
                }
                if (ImGui::MenuItem("Lua Script")) {
                    std::filesystem::path nf = m_CurrentDirectory / "NewScript.lua";
                    int c = 1; while (std::filesystem::exists(nf)) nf = m_CurrentDirectory / ("NewScript" + std::to_string(c++) + ".lua");
                    std::ofstream ofs(nf);
                    ofs << "-- Default Lua Script\n"
                        "function OnStart()\n"
                        "    print(\"Hello from Lua!\")\n"
                        "end\n\n"
                        "function OnUpdate(dt)\n"
                        "    -- Your update logic here\n"
                        "end\n";
                    ofs.close();
                }
                if (ImGui::MenuItem("Shader (.glsl)")) {
                    auto nf = m_CurrentDirectory / "NewShader.glsl";
                    int i = 1; while (std::filesystem::exists(nf)) nf = m_CurrentDirectory / ("NewShader" + std::to_string(i++) + ".glsl");
                    std::ofstream ofs(nf); ofs << "// vertex/fragment\n"; ofs.close();
                }
                if (ImGui::MenuItem("Material (.mat)")) {
                    auto nf = m_CurrentDirectory / "NewMaterial.mat";
                    int i = 1; while (std::filesystem::exists(nf)) nf = m_CurrentDirectory / ("NewMaterial" + std::to_string(i++) + ".mat");
                    std::ofstream ofs(nf); ofs << "albedo=1,1,1\nmetallic=0\nroughness=1\n"; ofs.close();
                }
                if (ImGui::MenuItem("Scene (.qscene)")) {
                    auto nf = m_CurrentDirectory / "NewScene.qscene";
                    int i = 1; while (std::filesystem::exists(nf)) nf = m_CurrentDirectory / ("NewScene" + std::to_string(i++) + ".qscene");
                    std::ofstream ofs(nf); ofs << "{ \"entities\": [] }\n"; ofs.close();
                }
                ImGui::EndMenu();
            }

            bool canPaste = m_Clipboard.mode != ClipMode::None && !m_Clipboard.items.empty();
            ImGui::BeginDisabled(!canPaste);
            if (ImGui::MenuItem("Paste")) {
                std::error_code ec;
                for (auto& src : m_Clipboard.items) {
                    auto dst = m_CurrentDirectory / src.filename();
                    if (m_Clipboard.mode == ClipMode::Copy) { ec.clear(); CopyRec(src, dst, ec); }
                    else if (m_Clipboard.mode == ClipMode::Cut) { ec.clear(); std::filesystem::rename(src, dst, ec); }
                }
                if (m_Clipboard.mode == ClipMode::Cut) { m_Clipboard = {}; }
            }
            ImGui::EndDisabled();

            if (ImGui::BeginMenu("Trash")) {
                if (ImGui::MenuItem("Open Trash Folder")) {
                    m_CurrentDirectory = m_TrashDir;
                    std::snprintf(m_PathBuffer, sizeof(m_PathBuffer), "%s", m_CurrentDirectory.string().c_str());
                }
                if (ImGui::MenuItem("Empty Trash")) {
                    std::error_code ec;
                    for (auto& e : std::filesystem::directory_iterator(m_TrashDir, ec)) {
                        std::filesystem::remove_all(e.path(), ec);
                    }
                }
                if (ImGui::MenuItem("Restore Selected (from Trash)")) {
                    std::error_code ec;
                    for (auto& s : m_Selection) {
                        std::filesystem::path p(s);
                        if (p.parent_path() == m_TrashDir) {
                            auto dst = m_BaseDirectory / p.filename();
                            std::filesystem::rename(p, dst, ec);
                        }
                    }
                    m_Selection.clear();
                }
                ImGui::EndMenu();
            }

            ImGui::EndPopup();
        }

        HandleKeyboardShortcuts(ImGui::IsWindowFocused());

        ImGui::EndChild();
        ImGui::End();
    }
}
