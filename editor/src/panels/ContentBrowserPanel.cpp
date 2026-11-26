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
        constexpr float FoldersWidth = 200.0f;

        bool IsInside(const std::filesystem::path& path, const std::filesystem::path& folder)
        {
            return MakeRelativePath(path, folder).has_value();
        }

        std::vector<ContentBrowserFolder> ReadFolders(const std::filesystem::path& assets)
        {
            std::vector<ContentBrowserFolder> folders{{.Path = assets}};
            std::error_code error;
            for (std::filesystem::recursive_directory_iterator it(assets, std::filesystem::directory_options::skip_permission_denied, error), end;
                 !error && it != end; it.increment(error))
            {
                std::error_code typeError;
                if (it->is_directory(typeError) && !it->is_symlink(typeError))
                {
                    folders.push_back({.Path = it->path(), .Depth = it.depth() + 1});
                }
            }

            std::ranges::sort(folders, {}, &ContentBrowserFolder::Path);
            for (std::size_t index = 0; index + 1 < folders.size(); ++index)
            {
                folders[index].HasChildren = folders[index + 1].Depth > folders[index].Depth;
            }
            return folders;
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
        ImGui::TableSetupColumn("Entries", ImGuiTableColumnFlags_WidthStretch);
        ImGui::TableSetupColumn("Folders", ImGuiTableColumnFlags_WidthFixed, UI::Scaled(FoldersWidth));
        ImGui::TableNextRow();

        ImGui::TableSetColumnIndex(0);
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

        ImGui::TableSetColumnIndex(1);
        if (ImGui::BeginChild("Folders"))
        {
            DrawFolders();
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
        m_Folders = m_AssetsDirectory.empty() ? std::vector<ContentBrowserFolder>{} : ReadFolders(m_AssetsDirectory);
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
        for (std::filesystem::directory_iterator it(m_ShownDirectory, error), end; !error && it != end; it.increment(error))
        {
            std::error_code typeError;
            m_Entries.push_back({it->path(), it->is_directory(typeError)});
        }
        if (error)
        {
            QUASAR_WARN("Cannot read the folder {}: {}", PathToUtf8(m_ShownDirectory), error.message());
            m_Entries.clear();
            return;
        }

        std::ranges::sort(m_Entries,
                          [](const ContentBrowserEntry& left, const ContentBrowserEntry& right)
                          {
                              if (left.IsDirectory != right.IsDirectory)
                              {
                                  return left.IsDirectory;
                              }
                              return left.Path.filename() < right.Path.filename();
                          });
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

    void ContentBrowserPanel::DrawFolders()
    {
        std::optional<std::filesystem::path> opened;
        int openDepth = 0;
        for (const ContentBrowserFolder& folder : m_Folders)
        {
            while (openDepth > folder.Depth)
            {
                ImGui::TreePop();
                --openDepth;
            }
            if (folder.Depth > openDepth)
            {
                continue;
            }

            ImGuiTreeNodeFlags flags = ImGuiTreeNodeFlags_OpenOnArrow | ImGuiTreeNodeFlags_OpenOnDoubleClick | ImGuiTreeNodeFlags_SpanAvailWidth;
            if (!folder.HasChildren)
            {
                flags |= ImGuiTreeNodeFlags_Leaf | ImGuiTreeNodeFlags_NoTreePushOnOpen;
            }
            if (folder.Depth == 0)
            {
                flags |= ImGuiTreeNodeFlags_DefaultOpen;
            }
            if (folder.Path == m_ShownDirectory)
            {
                flags |= ImGuiTreeNodeFlags_Selected;
            }

            if (m_RevealShownDirectory && folder.HasChildren && folder.Path != m_ShownDirectory && IsInside(m_ShownDirectory, folder.Path))
            {
                ImGui::SetNextItemOpen(true);
            }

            const std::string label = PathForDisplay(PathToUtf8(folder.Path.filename())) + "###" + PathToUtf8(folder.Path);
            const bool open = ImGui::TreeNodeEx(label.c_str(), flags);
            if (ImGui::IsItemClicked() && !ImGui::IsItemToggledOpen())
            {
                opened = folder.Path;
            }
            if (open && folder.HasChildren)
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
        if (opened)
        {
            OpenDirectory(*opened);
        }
    }
}
