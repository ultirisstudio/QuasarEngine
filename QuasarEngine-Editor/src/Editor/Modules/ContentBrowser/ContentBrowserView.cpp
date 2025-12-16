#include "ContentBrowserView.h"

#include "ContentBrowserModel.h"
#include "ContentBrowserActions.h"

#include <filesystem>
#include <vector>
#include <string>
#include <algorithm>
#include <cstring>
#include <chrono>
#include <ctime>
#include <sstream>
#include <iomanip>

#include "imgui/imgui.h"
#include <imgui/imgui_internal.h>

#include <QuasarEngine/Resources/Model.h>

namespace QuasarEngine
{
    namespace
    {
        static std::string FormatTimeShort(std::time_t tt)
        {
            std::tm tm{};
#ifdef _WIN32
            localtime_s(&tm, &tt);
#else
            localtime_r(&tt, &tm);
#endif
            std::ostringstream oss;
            oss << std::put_time(&tm, "%Y-%m-%d %H:%M");
            return oss.str();
        }
    }

    ContentBrowserView::ContentBrowserView(ContentBrowserModel& model, ContentBrowserActions& actions)
        : m_Model(model), m_Actions(actions)
    {
    }

    void ContentBrowserView::Render()
    {
        if (m_Model.CacheDirty())
            m_Model.RefreshDirectoryCache();

        ImGui::Begin("Content Browser");

        auto& ui = m_Model.UI();

        const float min_left = 160.0f;
        const float max_left = std::max(160.0f, ImGui::GetContentRegionAvail().x - 300.0f);

        ImGui::BeginChild("##left_tree_panel", ImVec2(ui.leftPaneWidth, 0), true, ImGuiWindowFlags_NoMove);
        DrawFolderTreePanel();
        ImGui::EndChild();

        ImGui::SameLine(0.0f, 0.0f);
        ImGui::InvisibleButton("##vsplitter", ImVec2(ui.splitterWidth, -1));
        if (ImGui::IsItemActive())
            ui.leftPaneWidth = std::clamp(ui.leftPaneWidth + ImGui::GetIO().MouseDelta.x, min_left, max_left);
        if (ImGui::IsItemHovered())
            ImGui::SetMouseCursor(ImGuiMouseCursor_ResizeEW);

        ImGui::SameLine(0.0f, 0.0f);
        ImGui::BeginChild("##right_content", ImVec2(0, 0), false);

        if (ImGui::IsWindowHovered(ImGuiHoveredFlags_AllowWhenBlockedByActiveItem) &&
            ImGui::IsMouseClicked(ImGuiMouseButton_Left) &&
            !ImGui::IsAnyItemHovered())
        {
            m_Model.ClearSelection();
        }

        if (!m_Model.LastError().empty()) {
            ImGui::TextColored(ImVec4(1, 0.4f, 0.4f, 1), "Error: %s", m_Model.LastError().c_str());
        }

        DrawBreadcrumbs();
        DrawToolbar();

        float panelWidth = ImGui::GetContentRegionAvail().x;
        float rightPaneW = ui.listView ? 0.0f : std::min(280.0f, panelWidth * 0.35f);
        float contentWidth = panelWidth;
        if (rightPaneW > 0.0f) contentWidth -= rightPaneW + ImGui::GetStyle().ItemSpacing.x;

        if (ui.listView) {
            DrawListView();
        }
        else {
            float cardPadding = 12.0f;
            float iconSize = ui.thumbSize;
            float cardW = iconSize + cardPadding * 2.0f;
            int columnCount = std::max(1, (int)std::floor(contentWidth / (cardW + ImGui::GetStyle().ItemSpacing.x)));
            DrawGridView(columnCount);
            DrawPreviewPane(rightPaneW);
        }

        DrawContextWindowPopup();
        DrawDeleteConfirmModal();

        HandleKeyboardShortcuts(ImGui::IsWindowFocused());

        ImGui::EndChild();
        ImGui::End();
    }

    void ContentBrowserView::DrawBreadcrumbs()
    {
        std::filesystem::path p = m_Model.CurrentDirectory().lexically_normal();

        std::filesystem::path root = p.root_path();
        std::filesystem::path accum = root;
        std::vector<std::filesystem::path> crumbs;

        if (!root.empty())
            crumbs.push_back(root);

        for (auto& part : p.relative_path()) {
            accum /= part;
            crumbs.push_back(accum);
        }

        ImGui::TextUnformatted("Path:");
        ImGui::SameLine();

        for (size_t i = 0; i < crumbs.size(); ++i)
        {
            if (i > 0) ImGui::SameLine(0.0f, 5.0f);

            std::string visible;
            if (i == 0 && !root.empty()) {
#if defined(_WIN32)
                visible = root.string();
#else
                visible = "/";
#endif
            }
            else {
                visible = crumbs[i].filename().string();
                if (visible.empty()) visible = "/";
            }

            std::string id = visible + "##crumb_" + std::to_string(i);
            if (ImGui::SmallButton(id.c_str()))
                m_Model.ChangeDirectory(crumbs[i]);

            if (i < crumbs.size() - 1) {
                ImGui::SameLine();
                ImGui::TextUnformatted(">");
                ImGui::SameLine();
            }
        }

        if (m_Model.CurrentDirectory() != m_Model.BaseDirectory())
        {
            ImGui::SameLine();
            if (ImGui::Button("<- Back"))
                m_Model.ChangeDirectory(m_Model.CurrentDirectory().parent_path());
        }
    }

    void ContentBrowserView::DrawToolbar()
    {
        auto& ui = m_Model.UI();
        bool changed = false;

        changed |= ImGui::Checkbox("Group by Type", &ui.groupByType);

        ImGui::SameLine();
        if (ImGui::Button(ui.listView ? "Grid" : "List")) ui.listView = !ui.listView;

        ImGui::SameLine();
        ImGui::SliderFloat("Thumb", &ui.thumbSize, 48.0f, 192.0f, "%.0f");

        ImGui::SameLine();
        if (ImGui::Button("Refresh")) m_Model.MarkCacheDirty();

        ImGui::SameLine();
        changed |= ImGui::Checkbox("Show hidden", &ui.showHidden);

        ImGui::SameLine();
        ImGui::Checkbox("Watch", &ui.watchEnabled);

        ImGui::SameLine();
        if (ImGui::InputTextWithHint("##search", "Search files...", ui.search, IM_ARRAYSIZE(ui.search)))
            changed = true;

        ImGui::SameLine();

        const char* sortLabel =
            ui.sortMode == ContentBrowserModel::SortMode::NameAsc ? "Name Up" :
            ui.sortMode == ContentBrowserModel::SortMode::NameDesc ? "Name Down" :
            ui.sortMode == ContentBrowserModel::SortMode::DateAsc ? "Date Up" : "Date Down";

        if (ImGui::BeginCombo("Sort By", sortLabel)) {
            if (ImGui::Selectable("Name Up")) { ui.sortMode = ContentBrowserModel::SortMode::NameAsc;  changed = true; }
            if (ImGui::Selectable("Name Down")) { ui.sortMode = ContentBrowserModel::SortMode::NameDesc; changed = true; }
            if (ImGui::Selectable("Date Up")) { ui.sortMode = ContentBrowserModel::SortMode::DateAsc;  changed = true; }
            if (ImGui::Selectable("Date Down")) { ui.sortMode = ContentBrowserModel::SortMode::DateDesc; changed = true; }
            ImGui::EndCombo();
        }

        const char* names[] = { "All","Textures","Meshes","Scripts","Scenes","Other" };
        ImGui::PushID("FilterBar");
        for (int i = 0; i < 6; ++i) {
            if (i == 0) ImGui::Separator();
            if (i) ImGui::SameLine();

            std::string label = std::string(names[i]) + "##filter_" + std::to_string(i);
            if (ImGui::RadioButton(label.c_str(), ui.typeFilter == i)) {
                ui.typeFilter = i;
                changed = true;
            }
        }
        ImGui::PopID();

        if (ImGui::InputText("##path", ui.path, IM_ARRAYSIZE(ui.path),
            ImGuiInputTextFlags_AutoSelectAll | ImGuiInputTextFlags_EnterReturnsTrue))
        {
            std::filesystem::path np(ui.path);
            m_Model.ChangeDirectory(np);
        }
        ImGui::SameLine();
        if (ImGui::Button("Go")) {
            std::filesystem::path np(ui.path);
            if (!m_Model.ChangeDirectory(np))
                std::snprintf(ui.path, sizeof(ui.path), "%s", m_Model.CurrentDirectory().string().c_str());
        }

        ImGui::SameLine();
        const auto& cb = m_Model.ClipboardState();
        bool canPaste = (cb.mode != ContentBrowserModel::ClipMode::None && !cb.items.empty());
        ImGui::BeginDisabled(!canPaste);
        if (ImGui::Button("Paste")) {
            m_Actions.PasteInto(m_Model.CurrentDirectory());
        }
        ImGui::EndDisabled();

        if (changed)
            m_Model.RebuildVisibleList();
    }

    void ContentBrowserView::DrawFolderTreePanel()
    {
        ImGui::TextDisabled("Folders");
        ImGui::Separator();
        DrawFolderNode(m_Model.BaseDirectory());
    }

    void ContentBrowserView::DrawFolderNode(const std::filesystem::path& dir)
    {
        auto& ui = m_Model.UI();

        std::error_code ec;
        if (!std::filesystem::exists(dir, ec) || !std::filesystem::is_directory(dir, ec))
            return;

        std::string label = dir.filename().string();
        if (label.empty()) label = dir.string();

        ImGuiTreeNodeFlags flags = ImGuiTreeNodeFlags_OpenOnArrow
            | ImGuiTreeNodeFlags_SpanFullWidth
            | ImGuiTreeNodeFlags_FramePadding;

        if (dir == m_Model.CurrentDirectory())
            flags |= ImGuiTreeNodeFlags_Selected;

        bool has_child = false;
        for (auto it = std::filesystem::directory_iterator(dir, ec);
            !ec && it != std::filesystem::directory_iterator(); ++it)
        {
            if (it->is_directory(ec)) {
                auto name = it->path().filename().string();
                if (!ui.showHidden && !name.empty() && name[0] == '.') continue;
                has_child = true; break;
            }
        }
        if (!has_child) flags |= ImGuiTreeNodeFlags_Leaf;

        ImGui::PushID(dir.generic_string().c_str());
        bool open = ImGui::TreeNodeEx(label.c_str(), flags);

        if (ImGui::IsItemClicked(ImGuiMouseButton_Left))
            m_Model.ChangeDirectory(dir);

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
                if (!src.empty() && src != dir)
                    m_Actions.MoveItemToDir(src, dir);
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
                if (!ui.showHidden && !name.empty() && name[0] == '.') continue;
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

    void ContentBrowserView::ItemContextMenu(const std::filesystem::path& absPath)
    {
        if (ImGui::MenuItem("Rename")) {
            m_Model.BeginRename(absPath);
        }
        if (ImGui::MenuItem("Copy")) {
            m_Model.SelectSingle(ContentBrowserModel::NormalizeKey(absPath));
            m_Actions.CopySelection();
        }
        if (ImGui::MenuItem("Cut")) {
            m_Model.SelectSingle(ContentBrowserModel::NormalizeKey(absPath));
            m_Actions.CutSelection();
        }

        const auto& cb = m_Model.ClipboardState();
        bool canPaste = (cb.mode != ContentBrowserModel::ClipMode::None && !cb.items.empty());
        ImGui::BeginDisabled(!canPaste);
        if (ImGui::MenuItem("Paste")) {
            std::error_code ec;
            auto dest = std::filesystem::is_directory(absPath, ec) ? absPath : absPath.parent_path();
            m_Actions.PasteInto(dest);
        }
        ImGui::EndDisabled();

        if (ImGui::MenuItem("Duplicate")) {
            m_Actions.Duplicate(absPath);
        }

        ImGui::Separator();

        if (ImGui::MenuItem("Reveal in Explorer")) {
            m_Actions.RevealInExplorer(absPath);
        }

        ImGui::Separator();

        if (ImGui::MenuItem("Delete")) {
            m_Model.RequestDelete({ absPath });
        }
    }

    void ContentBrowserView::DrawListView()
    {
        const auto& entries = m_Model.Entries();
        const auto& visible = m_Model.Visible();
        auto& ui = m_Model.UI();

        ImGuiTableFlags tflags = ImGuiTableFlags_SizingStretchProp | ImGuiTableFlags_RowBg |
            ImGuiTableFlags_BordersOuter | ImGuiTableFlags_BordersInnerH;

        if (!ImGui::BeginTable("##ContentList", 5, tflags))
            return;

        ImGui::TableSetupColumn("Name", ImGuiTableColumnFlags_WidthStretch, 0.48f);
        ImGui::TableSetupColumn("Type", ImGuiTableColumnFlags_WidthStretch, 0.14f);
        ImGui::TableSetupColumn("Size", ImGuiTableColumnFlags_WidthStretch, 0.12f);
        ImGui::TableSetupColumn("Modified", ImGuiTableColumnFlags_WidthStretch, 0.22f);
        ImGui::TableSetupColumn("", ImGuiTableColumnFlags_WidthFixed, 24.0f);
        ImGui::TableHeadersRow();

        const float rowHeight = 38.0f;
        const float icon24 = 24.0f;

        ContentBrowserModel::EntryGroup currentGroup = ContentBrowserModel::EntryGroup::Other;
        bool firstRow = true;

        for (size_t vi = 0; vi < visible.size(); ++vi)
        {
            const auto& e = entries[visible[vi]];

            if (ui.groupByType)
            {
                if (firstRow || e.group != currentGroup)
                {
                    currentGroup = e.group;
                    firstRow = false;

                    ImGui::TableNextRow();
                    ImGui::TableSetColumnIndex(0);
                    ImGui::TextDisabled("%s", ContentBrowserModel::GroupLabel(currentGroup));
                    for (int col = 1; col < 5; ++col) { ImGui::TableSetColumnIndex(col); ImGui::TextUnformatted(""); }
                }
            }

            bool isSelected = m_Model.IsSelected(e.key);

            ImGui::TableNextRow(ImGuiTableRowFlags_None, rowHeight);
            ImGui::TableSetColumnIndex(0);

            ImGuiSelectableFlags sflags = ImGuiSelectableFlags_SpanAllColumns |
                ImGuiSelectableFlags_AllowDoubleClick |
                ImGuiSelectableFlags_AllowItemOverlap;

            std::string rowId = "##row_" + e.key;
            if (ImGui::Selectable(rowId.c_str(), isSelected, sflags, ImVec2(0, rowHeight)))
            {
                if (ImGui::GetIO().KeyCtrl) m_Model.ToggleSelection(e.key);
                else m_Model.SelectSingle(e.key);

                if (ImGui::IsMouseDoubleClicked(ImGuiMouseButton_Left))
                    m_Actions.OpenOnDoubleClick(e);
            }

            if (ImGui::BeginPopupContextItem((std::string("##ctx_row_") + e.key).c_str()))
            {
                ItemContextMenu(e.absPath);
                ImGui::EndPopup();
            }

            ImVec2 startPos = ImGui::GetCursorPos();
            ImGui::SetCursorPosY(startPos.y - rowHeight + (rowHeight - icon24) * 0.5f);
            ImGui::Image((ImTextureID)e.icon->GetHandle(), ImVec2(icon24, icon24), ImVec2(0, 1), ImVec2(1, 0));
            ImGui::SameLine();

            if (m_Model.RenamingPath() && *m_Model.RenamingPath() == e.absPath)
            {
                ImGui::SetNextItemWidth(std::max(180.0f, ImGui::GetContentRegionAvail().x * 0.3f));
                ImGui::PushID(("##rename_" + e.key).c_str());

                bool commit = ImGui::InputText("##rename", m_Model.RenameBuffer(), (int)m_Model.RenameBufferSize(),
                    ImGuiInputTextFlags_EnterReturnsTrue | ImGuiInputTextFlags_AutoSelectAll);

                if (ImGui::IsItemActive() && ImGui::IsKeyPressed(ImGuiKey_Escape))
                    m_Model.CancelRename();

                if (commit)
                    m_Actions.CommitRename(e.absPath, std::string(m_Model.RenameBuffer()));

                ImGui::PopID();
            }
            else
            {
                ImGui::TextUnformatted(e.filename.c_str());
            }

            ImGui::TableSetColumnIndex(1);
            if (e.isDir) ImGui::TextUnformatted("Folder");
            else ImGui::TextUnformatted(e.ext.empty() ? "-" : e.ext.c_str());

            ImGui::TableSetColumnIndex(2);
            if (e.isDir) ImGui::TextUnformatted("-");
            else ImGui::TextUnformatted(ContentBrowserModel::PrettySize(e.size).c_str());

            ImGui::TableSetColumnIndex(3);
            {
                auto tt = ContentBrowserModel::ToTimeT(e.modified);
                ImGui::TextUnformatted(FormatTimeShort(tt).c_str());
            }

            ImGui::TableSetColumnIndex(4);

            if (e.isModel)
            {
                bool expanded = false;
                auto& map = m_Model.ExpandedModels();
                auto it = map.find(e.key);
                if (it != map.end()) expanded = it->second;

                std::string toggleId = "##toggle_row_" + e.key;
                if (ImGui::SmallButton(toggleId.c_str()))
                    map[e.key] = !expanded;
            }
        }

        ImGui::EndTable();
    }

    void ContentBrowserView::DrawGridView(int columnCount)
    {
        const auto& entries = m_Model.Entries();
        const auto& visible = m_Model.Visible();
        auto& ui = m_Model.UI();

        ImGuiTableFlags tflags = ImGuiTableFlags_SizingFixedFit |
            ImGuiTableFlags_NoBordersInBody |
            ImGuiTableFlags_BordersOuterV;

        auto drawCard = [&](const ContentBrowserModel::Entry& e)
            {
                float cardPadding = 12.0f;
                float textLines = 2.0f;
                float textH = ImGui::GetTextLineHeightWithSpacing() * textLines;
                float iconSize = ui.thumbSize;
                float cardW = iconSize + cardPadding * 2.0f;
                float cardH = iconSize + textH + cardPadding * 2.0f + 6.0f;
                float extraVGap = 12.0f;

                ImGui::TableNextColumn();
                ImGui::PushID(e.key.c_str());

                ImVec2 p0 = ImGui::GetCursorScreenPos();
                ImGui::InvisibleButton("##card_hit", ImVec2(cardW, cardH));
                ImGui::SetItemAllowOverlap();

                bool hovered = ImGui::IsItemHovered();
                bool leftClicked = ImGui::IsItemClicked(ImGuiMouseButton_Left);
                bool rightClicked = ImGui::IsItemClicked(ImGuiMouseButton_Right);
                bool doubleClicked = hovered && ImGui::IsMouseDoubleClicked(ImGuiMouseButton_Left);

                if (ImGui::BeginPopupContextItem("##item_ctx", ImGuiPopupFlags_MouseButtonRight))
                {
                    ItemContextMenu(e.absPath);
                    ImGui::EndPopup();
                }

                if (ImGui::IsItemActive() && ImGui::IsMouseDragging(ImGuiMouseButton_Left)) {
                    if (ImGui::BeginDragDropSource(ImGuiDragDropFlags_SourceAllowNullID)) {
#if defined(_WIN32)
                        const wchar_t* w = e.absPath.c_str();
                        ImGui::SetDragDropPayload("CONTENT_BROWSER_ITEM", w, (wcslen(w) + 1) * sizeof(wchar_t));
#else
                        std::string u8 = e.absPath.string();
                        ImGui::SetDragDropPayload("CONTENT_BROWSER_ITEM", u8.c_str(), u8.size() + 1);
#endif
                        ImGui::TextUnformatted(e.filename.c_str());
                        ImGui::EndDragDropSource();
                    }
                }

                ImDrawList* dl = ImGui::GetWindowDrawList();
                float rounding = 10.0f;

                ImVec2 cardMin = p0;
                ImVec2 cardMax = ImVec2(p0.x + cardW, p0.y + cardH);

                ImU32 colShadow = ImGui::GetColorU32(ImVec4(0, 0, 0, 0.10f));
                ImU32 colBg = ImGui::GetColorU32(hovered ? ImVec4(1, 1, 1, 0.06f) : ImVec4(1, 1, 1, 0.03f));
                ImU32 colBorder = ImGui::GetColorU32(ImVec4(1, 1, 1, 0.10f));

                dl->AddRectFilled(cardMin + ImVec2(0, 1), cardMax + ImVec2(0, 1), colShadow, rounding);
                dl->AddRectFilled(cardMin, cardMax, colBg, rounding);
                dl->AddRect(cardMin, cardMax, colBorder, rounding);

                ImVec2 iconPos(cardMin.x + cardPadding, cardMin.y + cardPadding);
                dl->AddImage((ImTextureID)e.icon->GetHandle(),
                    iconPos, iconPos + ImVec2(iconSize, iconSize),
                    ImVec2(0, 1), ImVec2(1, 0));

                const float textAreaW = cardW - cardPadding * 2.0f;
                ImVec2 textTop(cardMin.x + (cardW - textAreaW) * 0.5f,
                    iconPos.y + iconSize + 6.0f);

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

                            float x = cardMin.x + (cardW - line_w) * 0.5f;
                            dl->AddText(ImVec2(x, pos.y), ImGui::GetColorU32(ImVec4(1, 1, 1, 0.95f)), line_start, draw_end);
                            pos.y += line_h;
                        }
                    };

                if (m_Model.RenamingPath() && *m_Model.RenamingPath() == e.absPath)
                {
                    ImVec2 inputPos(cardMin.x + cardPadding, textTop.y);
                    ImGui::SetCursorScreenPos(inputPos);
                    ImGui::SetNextItemWidth(textAreaW);

                    ImGui::PushID(("##rename_" + e.key).c_str());

                    bool commit = ImGui::InputText(
                        "##rename",
                        m_Model.RenameBuffer(),
                        (int)m_Model.RenameBufferSize(),
                        ImGuiInputTextFlags_EnterReturnsTrue | ImGuiInputTextFlags_AutoSelectAll
                    );

                    if (ImGui::IsItemActive() && ImGui::IsKeyPressed(ImGuiKey_Escape))
                        m_Model.CancelRename();

                    if (commit)
                        m_Actions.CommitRename(e.absPath, std::string(m_Model.RenameBuffer()));

                    ImGui::PopID();
                }
                else
                {
                    drawCenteredWrap(e.displayName);
                }

                if (leftClicked) {
                    if (ImGui::GetIO().KeyCtrl) m_Model.ToggleSelection(e.key);
                    else m_Model.SelectSingle(e.key);
                }

                if (doubleClicked)
                    m_Actions.OpenOnDoubleClick(e);

                ImGui::SetCursorScreenPos(ImVec2(p0.x, p0.y + cardH + extraVGap));
                ImGui::Dummy(ImVec2(cardW, 0.0f));

                ImGui::PopID();
            };

        auto beginTable = [&](const char* label) -> bool
            {
                if (ui.groupByType) ImGui::SeparatorText(label);
                if (!ImGui::BeginTable((std::string("##ContentGrid_") + label).c_str(), columnCount, tflags))
                    return false;
                ImGui::PushStyleVar(ImGuiStyleVar_CellPadding, ImVec2(10.0f, 12.0f));
                return true;
            };

        auto endTable = [&]()
            {
                ImGui::PopStyleVar();
                ImGui::EndTable();
            };

        if (!ui.groupByType)
        {
            if (beginTable("Items")) {
                for (size_t idx : visible)
                    drawCard(entries[idx]);
                endTable();
            }
            return;
        }

        std::vector<ContentBrowserModel::EntryGroup> order = {
            ContentBrowserModel::EntryGroup::Folder,
            ContentBrowserModel::EntryGroup::Texture,
            ContentBrowserModel::EntryGroup::Model,
            ContentBrowserModel::EntryGroup::Script,
            ContentBrowserModel::EntryGroup::Scene,
            ContentBrowserModel::EntryGroup::Particle,
            ContentBrowserModel::EntryGroup::Other
        };

        for (auto g : order)
        {
            bool any = false;
            for (size_t idx : visible) { if (entries[idx].group == g) { any = true; break; } }
            if (!any) continue;

            const char* label = ContentBrowserModel::GroupLabel(g);
            if (!beginTable(label)) continue;

            for (size_t idx : visible)
            {
                if (entries[idx].group != g) continue;
                drawCard(entries[idx]);
            }

            endTable();
        }
    }

    void ContentBrowserView::DrawPreviewPane(float rightPaneWidth)
    {
        if (rightPaneWidth < 200.0f) return;
        auto sel = m_Model.SingleSelectionKey();
        if (!sel) return;

        ImGui::SameLine();
        ImGui::BeginChild("##preview", ImVec2(rightPaneWidth, 0), true);

        std::filesystem::path p(*sel);
        std::string key = ContentBrowserModel::NormalizeKey(p);

        ImGui::TextUnformatted("Preview");
        ImGui::Separator();
        ImGui::TextWrapped("%s", p.filename().string().c_str());

        std::error_code ec;
        bool isDir = std::filesystem::is_directory(p, ec);

        if (isDir) {
            ImGui::TextUnformatted("Folder");

            m_Model.RequestDirSizeForKey(key, p);
            uint64_t sz = 0;
            if (m_Model.TryGetDirSizeCached(key, sz)) {
                ImGui::Text("Size: %s", ContentBrowserModel::PrettySize(sz).c_str());
            }
            else if (m_Model.IsDirSizeComputing(key)) {
                ImGui::TextUnformatted("Size: Calculating...");
            }
            else {
                ImGui::TextUnformatted("Size: -");
            }
        }
        else {
            auto fsz = std::filesystem::file_size(p, ec);
            ImGui::TextUnformatted("File");
            ImGui::Text("Size: %s", ContentBrowserModel::PrettySize(ec ? 0 : (uint64_t)fsz).c_str());
            ImGui::Text("Ext: %s", p.extension().string().c_str());
        }

        auto tp = std::filesystem::last_write_time(p, ec);
        if (!ec) {
            auto tt = ContentBrowserModel::ToTimeT(tp);
            ImGui::Text("Modified: %s", FormatTimeShort(tt).c_str());
        }

        ImGui::EndChild();
    }

    void ContentBrowserView::DrawContextWindowPopup()
    {
        if (ImGui::BeginPopupContextWindow("ContentBrowserContext",
            ImGuiPopupFlags_MouseButtonRight | ImGuiPopupFlags_NoOpenOverItems))
        {
            if (ImGui::MenuItem("New Folder")) m_Actions.CreateNewFolder();

            if (ImGui::BeginMenu("New File")) {
                if (ImGui::MenuItem("Text File")) m_Actions.CreateNewTextFile();
                if (ImGui::MenuItem("Lua Script")) m_Actions.CreateNewLuaScript();
                if (ImGui::MenuItem("Shader (.glsl)")) m_Actions.CreateNewShader();
                if (ImGui::MenuItem("Material (.mat)")) m_Actions.CreateNewMaterial();
                if (ImGui::MenuItem("Scene (.qscene)")) m_Actions.CreateNewScene();
                if (ImGui::MenuItem("Particle Effect (.qparticle)")) m_Actions.CreateNewParticleEffect();
                ImGui::EndMenu();
            }

            const auto& cb = m_Model.ClipboardState();
            bool canPaste = (cb.mode != ContentBrowserModel::ClipMode::None && !cb.items.empty());
            ImGui::BeginDisabled(!canPaste);
            if (ImGui::MenuItem("Paste")) m_Actions.PasteInto(m_Model.CurrentDirectory());
            ImGui::EndDisabled();

            if (ImGui::BeginMenu("Trash")) {
                if (ImGui::MenuItem("Open Trash Folder")) m_Model.ChangeDirectory(m_Model.TrashDirectory());
                ImGui::EndMenu();
            }

            ImGui::EndPopup();
        }
    }

    void ContentBrowserView::DrawDeleteConfirmModal()
    {
        if (m_Model.ConsumeOpenDeleteConfirm())
            ImGui::OpenPopup("Confirm Delete");

        if (ImGui::BeginPopupModal("Confirm Delete", nullptr, ImGuiWindowFlags_AlwaysAutoResize))
        {
            ImGui::TextUnformatted("Move to Trash:");
            for (const auto& p : m_Model.PendingDelete())
                ImGui::BulletText("%s", p.filename().string().c_str());

            if (ImGui::Button("Confirm", ImVec2(120, 0))) {
                m_Actions.ConfirmDeleteToTrash();
                ImGui::CloseCurrentPopup();
            }
            ImGui::SameLine();
            if (ImGui::Button("Cancel", ImVec2(120, 0))) {
                m_Actions.CancelDelete();
                ImGui::CloseCurrentPopup();
            }

            ImGui::EndPopup();
        }
    }

    void ContentBrowserView::HandleKeyboardShortcuts(bool windowFocused)
    {
        if (!windowFocused) return;

        if (ImGui::IsKeyPressed(ImGuiKey_Backspace) && m_Model.CurrentDirectory() != m_Model.BaseDirectory())
            m_Model.ChangeDirectory(m_Model.CurrentDirectory().parent_path());

        if (ImGui::IsKeyDown(ImGuiMod_Ctrl) && ImGui::IsKeyPressed(ImGuiKey_C)) m_Actions.CopySelection();
        if (ImGui::IsKeyDown(ImGuiMod_Ctrl) && ImGui::IsKeyPressed(ImGuiKey_X)) m_Actions.CutSelection();
        if (ImGui::IsKeyDown(ImGuiMod_Ctrl) && ImGui::IsKeyPressed(ImGuiKey_V)) m_Actions.PasteInto(m_Model.CurrentDirectory());

        if (ImGui::IsKeyPressed(ImGuiKey_Delete)) {
            std::vector<std::filesystem::path> paths;
            paths.reserve(m_Model.Selection().size());
            for (const auto& k : m_Model.Selection())
                paths.push_back(std::filesystem::path(k));
            m_Model.RequestDelete(std::move(paths));
        }

        if (ImGui::IsKeyPressed(ImGuiKey_F2)) {
            auto sel = m_Model.SingleSelectionKey();
            if (sel) m_Model.BeginRename(std::filesystem::path(*sel));
        }

        if (ImGui::IsKeyPressed(ImGuiKey_Enter)) {
            auto sel = m_Model.SingleSelectionKey();
            if (!sel) return;
            std::filesystem::path p(*sel);
            std::error_code ec;
            if (std::filesystem::is_directory(p, ec))
                m_Model.ChangeDirectory(p);
        }
    }
}
