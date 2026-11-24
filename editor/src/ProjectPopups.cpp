#include "ProjectPopups.h"

#include "EditorUI.h"

#include <quasar/io/Path.h>

#include <imgui.h>
#include <imgui_stdlib.h>

#include <algorithm>
#include <cfloat>
#include <string>
#include <string_view>
#include <utility>

namespace Quasar::Editor
{
    using UI::AlignButtonsRight;
    using UI::DrawDisabledText;
    using UI::DrawScaledText;
    using UI::DrawSection;
    using UI::DrawSeparator;
    using UI::DrawText;
    using UI::IsCancelPressed;
    using UI::IsConfirmPressed;
    using UI::Scaled;

    namespace
    {
        constexpr const char* NewProjectPopupName = "New Project";
        constexpr const char* OpenErrorPopupName = "Cannot Open Project";
        constexpr const char* BrowseLabel = "...";
        constexpr const char* PlaceholderName = "MyGame";

        constexpr ImVec2 NewProjectMinimumSize{900.0f, 560.0f};
        constexpr float NewProjectViewportRatio = 0.6f;
        constexpr float MessagePopupWidth = 520.0f;
        constexpr float ButtonWidth = 110.0f;
        constexpr float TitleScale = 1.6f;
        constexpr float HeadingScale = 1.25f;
        constexpr float DetailsWidth = 300.0f;
        constexpr float NameColumnRatio = 0.35f;
        constexpr ImVec2 CardSize{150.0f, 170.0f};
        constexpr float CardRounding = 6.0f;
        constexpr float CardBorder = 2.0f;
        constexpr ImVec4 ErrorTextColor{0.898f, 0.325f, 0.294f, 1.0f};

        struct ProjectTemplate
        {
            const char* Name;
            const char* Description;
        };

        constexpr ProjectTemplate BlankTemplate{.Name = "Blank", .Description = "An empty project, with only an Assets folder for its files."};

        bool BeginNewProjectPopup()
        {
            const ImGuiViewport* viewport = ImGui::GetMainViewport();
            const ImVec2 minimumSize = Scaled(NewProjectMinimumSize);
            const ImVec2 size{std::max(minimumSize.x, viewport->WorkSize.x * NewProjectViewportRatio),
                              std::max(minimumSize.y, viewport->WorkSize.y * NewProjectViewportRatio)};
            ImGui::SetNextWindowPos(viewport->GetCenter(), ImGuiCond_Appearing, ImVec2(0.5f, 0.5f));
            ImGui::SetNextWindowSize(size, ImGuiCond_Appearing);
            ImGui::SetNextWindowSizeConstraints(minimumSize, ImVec2(FLT_MAX, FLT_MAX));

            ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, Scaled(UI::PopupPadding));
            const bool open = ImGui::BeginPopupModal(NewProjectPopupName, nullptr, ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_NoTitleBar);
            ImGui::PopStyleVar();
            return open;
        }

        void DrawFolderIcon(ImDrawList* drawList, ImVec2 center, float width, ImU32 color)
        {
            const float height = width * 0.75f;
            const ImVec2 bodyMin{center.x - (width / 2.0f), center.y - (height / 2.0f) + (height * 0.15f)};
            const ImVec2 bodyMax{center.x + (width / 2.0f), center.y + (height / 2.0f)};
            const ImVec2 tabMin{bodyMin.x, bodyMin.y - (height * 0.15f)};
            const float rounding = Scaled(CardRounding);
            const ImVec2 tabMax{bodyMin.x + (width * 0.4f), bodyMin.y + rounding};
            drawList->AddRectFilled(tabMin, tabMax, color, rounding / 2.0f);
            drawList->AddRectFilled(bodyMin, bodyMax, color, rounding / 2.0f);
        }

        bool DrawTemplateCard(const ProjectTemplate& projectTemplate, bool selected)
        {
            ImGui::PushID(projectTemplate.Name);
            const ImVec2 cardSize = Scaled(CardSize);
            const bool clicked = ImGui::InvisibleButton("Card", cardSize);
            const bool hovered = ImGui::IsItemHovered();
            ImGui::PopID();

            const ImVec2 min = ImGui::GetItemRectMin();
            const ImVec2 max = ImGui::GetItemRectMax();
            ImDrawList* drawList = ImGui::GetWindowDrawList();

            drawList->AddRectFilled(min, max, ImGui::GetColorU32(hovered ? ImGuiCol_FrameBgHovered : ImGuiCol_FrameBg), Scaled(CardRounding));
            if (selected)
            {
                drawList->AddRect(min, max, ImGui::GetColorU32(ImGuiCol_CheckMark), Scaled(CardRounding), 0, Scaled(CardBorder));
            }

            const float labelHeight = ImGui::GetTextLineHeightWithSpacing() * 1.5f;
            const ImVec2 iconCenter{(min.x + max.x) / 2.0f, min.y + ((cardSize.y - labelHeight) / 2.0f)};
            DrawFolderIcon(drawList, iconCenter, cardSize.x * 0.45f, ImGui::GetColorU32(ImGuiCol_TextDisabled));

            const ImVec2 labelSize = ImGui::CalcTextSize(projectTemplate.Name);
            const ImVec2 labelPosition{(min.x + max.x - labelSize.x) / 2.0f, max.y - labelHeight + ((labelHeight - labelSize.y) / 2.0f)};
            drawList->AddText(labelPosition, ImGui::GetColorU32(ImGuiCol_Text), projectTemplate.Name);

            return clicked;
        }

        void DrawTemplateDetails(const ProjectTemplate& projectTemplate, std::string_view projectName)
        {
            DrawScaledText(projectTemplate.Name, HeadingScale);
            ImGui::Spacing();
            DrawText(projectTemplate.Description);

            ImGui::Spacing();
            ImGui::Spacing();
            DrawSection("CREATES");
            const std::string name(projectName.empty() ? std::string_view(PlaceholderName) : projectName);
            ImGui::TextUnformatted((name + "/").c_str());
            ImGui::Indent();
            ImGui::TextUnformatted((std::string(AssetsDirectoryName) + "/").c_str());
            ImGui::Unindent();
        }

        float GetFooterHeight()
        {
            const ImGuiStyle& style = ImGui::GetStyle();
            return (ImGui::GetTextLineHeightWithSpacing() * 3.0f) + (ImGui::GetFrameHeightWithSpacing() * 2.0f) + (style.ItemSpacing.y * 4.0f);
        }
    }

    ProjectPopups::ProjectPopups(EditorSettings& settings, Scope<FileDialogBackend> dialogBackend) : m_Settings(&settings), m_Dialog(std::move(dialogBackend))
    {
    }

    void ProjectPopups::RequestNewProject()
    {
        m_NewProjectRequested = true;
        m_Name.clear();
        m_NewProjectError.clear();
        m_Location = PathToUtf8(GetLastLocation());
    }

    void ProjectPopups::RequestOpenProject()
    {
        OpenDialog(DialogPurpose::Project, GetLastLocation());
    }

    void ProjectPopups::RequestOpenProject(std::filesystem::path projectDirectory)
    {
        m_ProjectToOpen = std::move(projectDirectory);
    }

    std::optional<ProjectResult> ProjectPopups::Draw()
    {
        if (m_NewProjectRequested)
        {
            ImGui::OpenPopup(NewProjectPopupName);
            m_NewProjectRequested = false;
        }
        ReceiveDialogFolder();

        std::optional<ProjectResult> project = DrawNewProjectPopup();
        if (!project)
        {
            project = OpenRequestedProject();
        }
        DrawOpenErrorPopup();

        if (project)
        {
            m_Settings->RememberProject(project->Directory);
        }
        return project;
    }

    std::filesystem::path ProjectPopups::GetLastLocation() const
    {
        const std::filesystem::path& location = m_Settings->GetLastLocation();
        return location.empty() ? GetDefaultProjectLocation() : location;
    }

    void ProjectPopups::OpenDialog(DialogPurpose purpose, const std::filesystem::path& location)
    {
        if (m_Dialog.IsOpen())
        {
            return;
        }
        m_DialogPurpose = purpose;
        m_Dialog.OpenFolder(m_ParentWindow, FindExistingDirectory(location));
    }

    void ProjectPopups::ReceiveDialogFolder()
    {
        std::optional<std::filesystem::path> folder = m_Dialog.TakeFolder();
        if (!folder)
        {
            return;
        }
        if (m_DialogPurpose == DialogPurpose::Location)
        {
            m_Location = PathToUtf8(*folder);
            m_NewProjectError.clear();
        }
        else if (m_DialogPurpose == DialogPurpose::Project)
        {
            m_ProjectToOpen = std::move(folder);
        }
        m_DialogPurpose = DialogPurpose::None;
    }

    std::optional<ProjectResult> ProjectPopups::DrawNewProjectPopup()
    {
        if (!BeginNewProjectPopup())
        {
            return std::nullopt;
        }

        DrawNewProjectHeader();
        DrawNewProjectBody();
        std::optional<ProjectResult> created = DrawNewProjectFooter();

        ImGui::EndPopup();
        return created;
    }
    void ProjectPopups::DrawNewProjectHeader()
    {
        DrawScaledText("New Project", TitleScale);
        DrawDisabledText("Choose a template, a name and a location");
        DrawSeparator();
    }

    void ProjectPopups::DrawNewProjectBody() const
    {
        constexpr ImGuiTableFlags flags = ImGuiTableFlags_BordersInnerV | ImGuiTableFlags_PadOuterX;
        if (!ImGui::BeginTable("Body", 2, flags, ImVec2(0.0f, -GetFooterHeight())))
        {
            return;
        }

        ImGui::TableSetupColumn("Templates", ImGuiTableColumnFlags_WidthStretch);
        ImGui::TableSetupColumn("Details", ImGuiTableColumnFlags_WidthFixed, Scaled(DetailsWidth));
        ImGui::TableNextRow();

        ImGui::TableSetColumnIndex(0);
        DrawSection("TEMPLATES");
        DrawTemplateCard(BlankTemplate, true);

        ImGui::TableSetColumnIndex(1);
        DrawTemplateDetails(BlankTemplate, m_Name);

        ImGui::EndTable();
    }

    std::optional<ProjectResult> ProjectPopups::DrawNewProjectFooter()
    {
        DrawSeparator();

        const ImGuiStyle& style = ImGui::GetStyle();
        if (ImGui::BeginTable("Fields", 2, ImGuiTableFlags_None))
        {
            ImGui::TableSetupColumn("Location", ImGuiTableColumnFlags_WidthStretch, 1.0f - NameColumnRatio);
            ImGui::TableSetupColumn("Name", ImGuiTableColumnFlags_WidthStretch, NameColumnRatio);

            ImGui::TableNextRow();
            ImGui::TableSetColumnIndex(0);
            ImGui::TextUnformatted("Project location");
            ImGui::TableSetColumnIndex(1);
            ImGui::TextUnformatted("Project name");

            ImGui::TableNextRow();
            ImGui::TableSetColumnIndex(0);
            const float browseWidth = ImGui::GetFrameHeight() + (style.FramePadding.x * 2.0f);
            ImGui::SetNextItemWidth(ImGui::GetContentRegionAvail().x - browseWidth - style.ItemSpacing.x);
            if (ImGui::InputText("##Location", &m_Location))
            {
                m_NewProjectError.clear();
            }
            ImGui::SameLine();
            ImGui::BeginDisabled(m_Dialog.IsOpen());
            if (ImGui::Button(BrowseLabel, ImVec2(browseWidth, 0.0f)))
            {
                OpenDialog(DialogPurpose::Location, PathFromUtf8(m_Location));
            }
            ImGui::EndDisabled();
            if (ImGui::BeginItemTooltip())
            {
                ImGui::TextUnformatted("Browse");
                ImGui::EndTooltip();
            }

            ImGui::TableSetColumnIndex(1);
            if (ImGui::IsWindowAppearing())
            {
                ImGui::SetKeyboardFocusHere();
            }
            ImGui::SetNextItemWidth(-FLT_MIN);
            if (ImGui::InputTextWithHint("##Name", PlaceholderName, &m_Name))
            {
                m_NewProjectError.clear();
            }

            ImGui::EndTable();
        }

        const std::filesystem::path directory = ResolveProjectDirectory(PathFromUtf8(m_Location), m_Name);
        DrawDisabledText(directory.empty() ? std::string("Enter a name and the full path of the location") : PathToUtf8(directory));

        if (m_NewProjectError.empty())
        {
            ImGui::Dummy(ImVec2(0.0f, ImGui::GetTextLineHeight()));
        }
        else
        {
            UI::DrawWrappedText(ErrorTextColor, m_NewProjectError);
        }

        std::optional<ProjectResult> created;
        AlignButtonsRight(2, Scaled(ButtonWidth));
        if (ImGui::Button("Cancel", ImVec2(Scaled(ButtonWidth), 0.0f)) || IsCancelPressed())
        {
            ImGui::CloseCurrentPopup();
        }
        ImGui::SameLine();
        const bool canCreate = !m_Name.empty();
        ImGui::BeginDisabled(!canCreate);
        const bool createClicked = ImGui::Button("Create", ImVec2(Scaled(ButtonWidth), 0.0f));
        ImGui::EndDisabled();

        if (canCreate && (createClicked || IsConfirmPressed()))
        {
            ProjectResult result = CreateProject(PathFromUtf8(m_Location), m_Name);
            if (result.IsValid())
            {
                created = std::move(result);
                ImGui::CloseCurrentPopup();
            }
            else
            {
                m_NewProjectError = std::move(result.Error);
            }
        }
        return created;
    }

    std::optional<ProjectResult> ProjectPopups::OpenRequestedProject()
    {
        if (!m_ProjectToOpen)
        {
            return std::nullopt;
        }

        ProjectResult result = OpenProject(*m_ProjectToOpen);
        if (!result.IsValid())
        {
            m_Settings->ForgetProject(*m_ProjectToOpen);
            m_ProjectToOpen.reset();
            m_OpenError = std::move(result.Error);
            ImGui::OpenPopup(OpenErrorPopupName);
            return std::nullopt;
        }

        m_ProjectToOpen.reset();
        return result;
    }

    void ProjectPopups::DrawOpenErrorPopup()
    {
        if (!UI::BeginMessagePopup(OpenErrorPopupName, Scaled(MessagePopupWidth)))
        {
            return;
        }

        DrawText(m_OpenError);
        DrawSeparator();

        AlignButtonsRight(1, Scaled(ButtonWidth));
        if (ImGui::Button("OK", ImVec2(Scaled(ButtonWidth), 0.0f)) || IsConfirmPressed() || IsCancelPressed())
        {
            ImGui::CloseCurrentPopup();
        }

        ImGui::EndPopup();
    }
}
