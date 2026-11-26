#include "panels/ContentBrowserPanel.h"

#include "EditorContext.h"
#include "EditorText.h"
#include "EditorUI.h"
#include "Project.h"

#include <quasar/io/Path.h>
#include <quasar/log/Log.h>

#include <imgui.h>

#include <algorithm>
#include <cstddef>
#include <optional>
#include <string>
#include <system_error>

namespace Quasar::Editor
{
    namespace
    {
        constexpr ImVec2 InitialSize{640.0f, 320.0f};
        constexpr ImVec2 CardSize{96.0f, 112.0f};
        constexpr float TreeWidth = 200.0f;

        constexpr float ShownFolderAlpha = 0.35f;
        constexpr float ShownFolderHoveredAlpha = 0.5f;
        constexpr float ShownFolderMarkWidth = 3.0f;

        constexpr float TreeIconScale = 0.8f;

        ImVec4 WithAlpha(ImVec4 color, float alpha)
        {
            return {color.x, color.y, color.z, alpha};
        }

        void PushShownFolderColors(ImVec4 accent)
        {
            ImGui::PushStyleColor(ImGuiCol_Header, WithAlpha(accent, ShownFolderAlpha));
            ImGui::PushStyleColor(ImGuiCol_HeaderHovered, WithAlpha(accent, ShownFolderHoveredAlpha));
            ImGui::PushStyleColor(ImGuiCol_HeaderActive, WithAlpha(accent, ShownFolderHoveredAlpha));
        }

        void FinishShownFolder(ImVec4 accent, bool scrollToIt)
        {
            ImGui::PopStyleColor(3);
            const ImVec2 min = ImGui::GetItemRectMin();
            ImGui::GetWindowDrawList()->AddRectFilled(min, ImVec2{min.x + UI::Scaled(ShownFolderMarkWidth), ImGui::GetItemRectMax().y},
                                                      ImGui::GetColorU32(accent));
            if (scrollToIt)
            {
                ImGui::SetScrollHereY();
            }
        }

        bool IsInside(const std::filesystem::path& path, const std::filesystem::path& folder)
        {
            return MakeRelativePath(path, folder).has_value();
        }

        std::vector<ContentBrowserEntry> ReadEntries(const std::filesystem::path& directory, std::error_code& error)
        {
            std::vector<ContentBrowserEntry> entries;
            for (std::filesystem::directory_iterator it(directory, error), end; !error && it != end; it.increment(error))
            {
                std::error_code typeError;
                entries.push_back({it->path(), it->is_directory(typeError)});
            }
            if (error)
            {
                return {};
            }

            std::ranges::sort(entries,
                              [](const ContentBrowserEntry& left, const ContentBrowserEntry& right)
                              {
                                  if (left.IsDirectory != right.IsDirectory)
                                  {
                                      return left.IsDirectory;
                                  }
                                  return left.Path.filename() < right.Path.filename();
                              });
            return entries;
        }

        std::vector<ContentBrowserTreeNode> ReadTree(const std::filesystem::path& assets)
        {
            struct Level
            {
                std::vector<ContentBrowserEntry> Entries;
                std::size_t Next = 0;
            };

            std::vector<ContentBrowserTreeNode> tree{{.Path = assets, .IsDirectory = true}};
            std::error_code error;
            std::vector<Level> levels{{.Entries = ReadEntries(assets, error)}};
            while (!levels.empty())
            {
                if (levels.back().Next == levels.back().Entries.size())
                {
                    levels.pop_back();
                    continue;
                }

                const ContentBrowserEntry entry = levels.back().Entries[levels.back().Next++];
                tree.push_back({.Path = entry.Path, .Depth = static_cast<int>(levels.size()), .IsDirectory = entry.IsDirectory});

                std::error_code linkError;
                if (entry.IsDirectory && !std::filesystem::is_symlink(entry.Path, linkError))
                {
                    levels.push_back({.Entries = ReadEntries(entry.Path, error)});
                }
            }

            for (std::size_t index = 0; index + 1 < tree.size(); ++index)
            {
                tree[index].HasChildren = tree[index + 1].Depth > tree[index].Depth;
            }
            return tree;
        }

        ImGuiTreeNodeFlags GetNodeFlags(const ContentBrowserTreeNode& node, bool highlighted)
        {
            ImGuiTreeNodeFlags flags = ImGuiTreeNodeFlags_OpenOnArrow | ImGuiTreeNodeFlags_OpenOnDoubleClick | ImGuiTreeNodeFlags_SpanFullWidth;
            if (!node.HasChildren)
            {
                flags |= ImGuiTreeNodeFlags_Leaf | ImGuiTreeNodeFlags_NoTreePushOnOpen;
            }
            if (node.Depth == 0)
            {
                flags |= ImGuiTreeNodeFlags_DefaultOpen;
            }
            if (highlighted)
            {
                flags |= ImGuiTreeNodeFlags_Selected;
            }
            return flags;
        }

        void DrawNodeLabel(const ContentBrowserTreeNode& node)
        {
            const float size = ImGui::GetTextLineHeight();
            ImGui::SameLine();
            ImGui::Dummy(ImVec2{size, size});
            const ImVec2 min = ImGui::GetItemRectMin();
            const ImVec2 center{min.x + (size / 2.0f), min.y + (size / 2.0f)};
            ImDrawList* drawList = ImGui::GetWindowDrawList();
            if (node.IsDirectory)
            {
                UI::DrawFolderIcon(drawList, center, size * TreeIconScale, ImGui::GetColorU32(ImGuiCol_CheckMark));
            }
            else
            {
                UI::DrawFileIcon(drawList, center, size * TreeIconScale, ImGui::GetColorU32(ImGuiCol_TextDisabled));
            }
            ImGui::SameLine();
            ImGui::TextUnformatted(PathForDisplay(PathToUtf8(node.Path.filename())).c_str());
        }
    }

    ContentBrowserPanel::ContentBrowserPanel(EditorContext& context) : EditorPanel("panel.content_browser", context)
    {
        SetInitialSize(InitialSize);
    }

    void ContentBrowserPanel::OnUpdate([[maybe_unused]] Timestep timestep)
    {
        FollowProject();
    }

    bool ContentBrowserPanel::OpenDirectory(const std::filesystem::path& directory)
    {
        std::error_code error;
        if (m_AssetsDirectory.empty() || !IsInside(directory, m_AssetsDirectory) || !std::filesystem::is_directory(directory, error))
        {
            return false;
        }

        m_ShownDirectory = directory;
        m_RevealShownDirectory = true;
        ReadShownDirectory();
        return true;
    }

    void ContentBrowserPanel::OnImGuiRender()
    {
        if (m_ShownDirectory.empty())
        {
            UI::DrawDisabledText(Tr("content_browser.no_project"));
            return;
        }

        if (!ImGui::BeginTable("Layout", 2, ImGuiTableFlags_Resizable | ImGuiTableFlags_BordersInnerV))
        {
            return;
        }
        ImGui::TableSetupColumn("Tree", ImGuiTableColumnFlags_WidthFixed, UI::Scaled(TreeWidth));
        ImGui::TableSetupColumn("Entries", ImGuiTableColumnFlags_WidthStretch);
        ImGui::TableNextRow();

        ImGui::TableSetColumnIndex(0);
        if (ImGui::BeginChild("Tree"))
        {
            DrawTree();
        }
        ImGui::EndChild();

        ImGui::TableSetColumnIndex(1);
        DrawToolbar();
        if (ImGui::BeginChild("Entries"))
        {
            if (m_Entries.empty())
            {
                UI::DrawDisabledText(Tr("content_browser.empty_folder"));
            }
            else
            {
                DrawEntries();
            }
        }
        ImGui::EndChild();

        ImGui::EndTable();
    }

    void ContentBrowserPanel::FollowProject()
    {
        const std::filesystem::path& project = GetContext().ProjectDirectory;
        if (project == m_ProjectDirectory)
        {
            return;
        }

        m_ProjectDirectory = project;
        m_AssetsDirectory = project.empty() ? std::filesystem::path{} : GetAssetsDirectory(project);
        m_Tree = m_AssetsDirectory.empty() ? std::vector<ContentBrowserTreeNode>{} : ReadTree(m_AssetsDirectory);
        m_ShownDirectory = m_AssetsDirectory;
        m_RevealShownDirectory = true;
        ReadShownDirectory();
    }

    void ContentBrowserPanel::ReadShownDirectory()
    {
        m_Entries.clear();
        m_Selected.clear();
        if (m_ShownDirectory.empty())
        {
            return;
        }

        std::error_code error;
        m_Entries = ReadEntries(m_ShownDirectory, error);
        if (error)
        {
            QUASAR_WARN("Cannot read the folder {}: {}", PathToUtf8(m_ShownDirectory), error.message());
        }
    }

    void ContentBrowserPanel::DrawToolbar()
    {
        ImGui::BeginDisabled(m_ShownDirectory == m_AssetsDirectory);
        if (ImGui::ArrowButton("##ParentFolder", ImGuiDir_Up))
        {
            OpenDirectory(m_ShownDirectory.parent_path());
        }
        ImGui::EndDisabled();
        if (ImGui::BeginItemTooltip())
        {
            ImGui::TextUnformatted(Tr("content_browser.parent_folder"));
            ImGui::EndTooltip();
        }
    }

    void ContentBrowserPanel::DrawEntries()
    {
        const ImVec2 cardSize = UI::Scaled(CardSize);
        const float spacing = ImGui::GetStyle().ItemSpacing.x;
        const int columns = std::max(1, static_cast<int>((ImGui::GetContentRegionAvail().x + spacing) / (cardSize.x + spacing)));

        const ImU32 folderColor = ImGui::GetColorU32(ImGuiCol_CheckMark);
        const ImU32 fileColor = ImGui::GetColorU32(ImGuiCol_TextDisabled);

        std::optional<std::filesystem::path> opened;
        int column = 0;
        for (const ContentBrowserEntry& entry : m_Entries)
        {
            if (column > 0) {
                ImGui::SameLine();
            }
            column = (column + 1) % columns;

            const std::string name = PathToUtf8(entry.Path.filename());
            const UI::CardIcon icon = entry.IsDirectory ? UI::CardIcon::Folder : UI::CardIcon::File;
            if (UI::DrawCard(name.c_str(), PathForDisplay(name), cardSize, icon, entry.IsDirectory ? folderColor : fileColor, entry.Path == m_Selected))
            {
                m_Selected = entry.Path;
            }
            if (entry.IsDirectory && ImGui::IsItemHovered() && ImGui::IsMouseDoubleClicked(ImGuiMouseButton_Left))
            {
                opened = entry.Path;
            }
        }

        if (opened)
        {
            OpenDirectory(*opened);
        }
    }

    void ContentBrowserPanel::DrawTree()
    {
        const ImVec4 accent = ImGui::GetStyleColorVec4(ImGuiCol_CheckMark);
        std::optional<std::filesystem::path> clicked;
        int openDepth = 0;
        for (const ContentBrowserTreeNode& node : m_Tree)
        {
            while (openDepth > node.Depth)
            {
                ImGui::TreePop();
                --openDepth;
            }
            if (node.Depth > openDepth)
            {
                continue;
            }

            const bool shown = node.Path == m_ShownDirectory;
            if (shown)
            {
                PushShownFolderColors(accent);
            }
            if (m_RevealShownDirectory && node.HasChildren && IsInside(m_ShownDirectory, node.Path))
            {
                ImGui::SetNextItemOpen(true);
            }

            const std::string id = "##" + PathToUtf8(node.Path);
            const bool open = ImGui::TreeNodeEx(id.c_str(), GetNodeFlags(node, shown || node.Path == m_Selected));
            if (ImGui::IsItemClicked() && !ImGui::IsItemToggledOpen())
            {
                clicked = node.Path;
            }
            if (shown)
            {
                FinishShownFolder(accent, m_RevealShownDirectory);
            }
            DrawNodeLabel(node);

            if (open && node.HasChildren)
            {
                ++openDepth;
            }
        }
        while (openDepth > 0)
        {
            ImGui::TreePop();
            --openDepth;
        }

        m_RevealShownDirectory = false;
        if (!clicked)
        {
            return;
        }
        if (!OpenDirectory(*clicked) && OpenDirectory(clicked->parent_path()))
        {
            m_Selected = *clicked;
        }
    }
}
