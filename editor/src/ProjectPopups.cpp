#include "ProjectPopups.h"

#include "Project.h"

#include <quasar/core/Application.h>
#include <quasar/io/Path.h>

#include <imgui.h>

#include <algorithm>
#include <cfloat>
#include <string_view>
#include <system_error>
#include <utility>

namespace Quasar::Editor
{
    namespace
    {
        constexpr const char* NewProjectPopupName = "New Project";
        constexpr const char* OpenErrorPopupName = "Cannot Open Project";
        constexpr const char* BrowseLabel = "...";
        constexpr const char* PlaceholderName = "MyGame";

        constexpr ImVec2 PopupPadding{24.0f, 20.0f};
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

        float Scaled(float size)
        {
            return size * ImGui::GetStyle().FontScaleDpi;
        }

        ImVec2 Scaled(ImVec2 size)
        {
            return {Scaled(size.x), Scaled(size.y)};
        }

        struct ProjectTemplate
        {
            const char* Name;
            const char* Description;
        };

        constexpr ProjectTemplate BlankTemplate{.Name = "Blank", .Description = "An empty project, with only an Assets folder for its files."};

        template <std::size_t Size>
        void CopyToBuffer(std::array<char, Size>& buffer, std::string_view text)
        {
            buffer.fill('\0');
            const std::size_t count = std::min(text.size(), Size - 1);
            std::copy_n(text.begin(), count, buffer.begin());
        }

        std::filesystem::path FindExistingFolder(std::filesystem::path path)
        {
            std::error_code error;
            while (!path.empty() && !std::filesystem::is_directory(path, error))
            {
                std::filesystem::path parent = path.parent_path();
                if (parent == path)
                {
                    return {};
                }
                path = std::move(parent);
            }
            return path;
        }

        SDL_Window* GetMainWindow()
        {
            return Application::Get().GetWindow().GetNativeWindow();
        }

        bool BeginNewProjectPopup()
        {
            const ImGuiViewport* viewport = ImGui::GetMainViewport();
            const ImVec2 minimumSize = Scaled(NewProjectMinimumSize);
            const ImVec2 size{std::max(minimumSize.x, viewport->WorkSize.x * NewProjectViewportRatio),
                              std::max(minimumSize.y, viewport->WorkSize.y * NewProjectViewportRatio)};
            ImGui::SetNextWindowPos(viewport->GetCenter(), ImGuiCond_Appearing, ImVec2(0.5f, 0.5f));
            ImGui::SetNextWindowSize(size, ImGuiCond_Appearing);
            ImGui::SetNextWindowSizeConstraints(minimumSize, ImVec2(FLT_MAX, FLT_MAX));

            ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, Scaled(PopupPadding));
            const bool open = ImGui::BeginPopupModal(NewProjectPopupName, nullptr, ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_NoTitleBar);
            ImGui::PopStyleVar();
            return open;
        }

        bool BeginMessagePopup(const char* name)
        {
            ImGui::SetNextWindowPos(ImGui::GetMainViewport()->GetCenter(), ImGuiCond_Appearing, ImVec2(0.5f, 0.5f));
            ImGui::SetNextWindowSize(ImVec2(Scaled(MessagePopupWidth), 0.0f));
            ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, Scaled(PopupPadding));
            const bool open = ImGui::BeginPopupModal(name, nullptr, ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoSavedSettings);
            ImGui::PopStyleVar();
            return open;
        }

        void DrawWrappedText(const ImVec4& color, const std::string& text)
        {
            ImGui::PushStyleColor(ImGuiCol_Text, color);
            ImGui::PushTextWrapPos(0.0f);
            ImGui::TextUnformatted(text.c_str());
            ImGui::PopTextWrapPos();
            ImGui::PopStyleColor();
        }

        void DrawDisabledText(const std::string& text)
        {
            DrawWrappedText(ImGui::GetStyleColorVec4(ImGuiCol_TextDisabled), text);
        }

        void DrawScaledText(const char* text, float scale)
        {
            ImGui::PushFont(nullptr, ImGui::GetStyle().FontSizeBase * scale);
            ImGui::TextUnformatted(text);
            ImGui::PopFont();
        }

        void DrawSection(const char* title)
        {
            DrawDisabledText(title);
            ImGui::Spacing();
        }

        void DrawFooterSeparator()
        {
            ImGui::Spacing();
            ImGui::Separator();
            ImGui::Spacing();
        }

        void AlignButtonsRight(int count)
        {
            const float width = (Scaled(ButtonWidth) * static_cast<float>(count)) + (ImGui::GetStyle().ItemSpacing.x * static_cast<float>(count - 1));
            ImGui::SetCursorPosX(ImGui::GetCursorPosX() + std::max(0.0f, ImGui::GetContentRegionAvail().x - width));
        }

        bool IsConfirmPressed()
        {
            return ImGui::IsKeyPressed(ImGuiKey_Enter) || ImGui::IsKeyPressed(ImGuiKey_KeypadEnter);
        }

        bool IsCancelPressed()
        {
            return ImGui::IsKeyPressed(ImGuiKey_Escape);
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
            DrawWrappedText(ImGui::GetStyleColorVec4(ImGuiCol_Text), projectTemplate.Description);

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

    void ProjectPopups::RequestNewProject()
    {
        m_NewProjectRequested = true;
        m_Name.fill('\0');
        m_NewProjectError.clear();
        CopyToBuffer(m_Location, PathToUtf8(GetLastLocation()));
    }

    void ProjectPopups::RequestOpenProject()
    {
        m_ProjectDialog.Open(GetMainWindow(), FindExistingFolder(GetLastLocation()));
    }

    std::optional<std::filesystem::path> ProjectPopups::Draw()
    {
        if (m_NewProjectRequested)
        {
            ImGui::OpenPopup(NewProjectPopupName);
            m_NewProjectRequested = false;
        }

        std::optional<std::filesystem::path> project = DrawNewProjectPopup();
        if (!project)
        {
            project = ReceiveOpenedProject();
        }
        DrawOpenErrorPopup();
        return project;
    }

    const std::filesystem::path& ProjectPopups::GetLastLocation()
    {
        if (m_LastLocation.empty())
        {
            m_LastLocation = GetDefaultProjectLocation();
        }
        return m_LastLocation;
    }

    std::optional<std::filesystem::path> ProjectPopups::DrawNewProjectPopup()
    {
        if (const std::optional<std::filesystem::path> folder = m_LocationDialog.TakeFolder())
        {
            CopyToBuffer(m_Location, PathToUtf8(*folder));
            m_NewProjectError.clear();
        }
        if (std::string error = m_LocationDialog.TakeError(); !error.empty())
        {
            m_NewProjectError = std::move(error);
        }

        if (!BeginNewProjectPopup())
        {
            return std::nullopt;
        }

        DrawNewProjectHeader();
        DrawNewProjectBody();
        std::optional<std::filesystem::path> created = DrawNewProjectFooter();

        ImGui::EndPopup();
        return created;
    }

    void ProjectPopups::DrawNewProjectHeader()
    {
        DrawScaledText("New Project", TitleScale);
        DrawDisabledText("Choose a template, a name and a location");
        DrawFooterSeparator();
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
        DrawTemplateDetails(BlankTemplate, m_Name.data());

        ImGui::EndTable();
    }

    std::optional<std::filesystem::path> ProjectPopups::DrawNewProjectFooter()
    {
        DrawFooterSeparator();

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
            if (ImGui::InputText("##Location", m_Location.data(), m_Location.size()))
            {
                m_NewProjectError.clear();
            }
            ImGui::SameLine();
            ImGui::BeginDisabled(m_LocationDialog.IsOpen());
            if (ImGui::Button(BrowseLabel, ImVec2(browseWidth, 0.0f)))
            {
                m_LocationDialog.Open(GetMainWindow(), FindExistingFolder(PathFromUtf8(m_Location.data())));
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
            if (ImGui::InputTextWithHint("##Name", PlaceholderName, m_Name.data(), m_Name.size()))
            {
                m_NewProjectError.clear();
            }

            ImGui::EndTable();
        }

        const std::filesystem::path directory = ResolveProjectDirectory(PathFromUtf8(m_Location.data()), m_Name.data());
        DrawDisabledText(directory.empty() ? std::string("Enter a name and the full path of the location") : PathToUtf8(directory));

        if (m_NewProjectError.empty())
        {
            ImGui::Dummy(ImVec2(0.0f, ImGui::GetTextLineHeight()));
        }
        else
        {
            DrawWrappedText(ErrorTextColor, m_NewProjectError);
        }

        std::optional<std::filesystem::path> created;
        AlignButtonsRight(2);
        if (ImGui::Button("Cancel", ImVec2(Scaled(ButtonWidth), 0.0f)) || IsCancelPressed())
        {
            ImGui::CloseCurrentPopup();
        }
        ImGui::SameLine();
        const bool canCreate = m_Name.front() != '\0';
        ImGui::BeginDisabled(!canCreate);
        const bool createClicked = ImGui::Button("Create", ImVec2(Scaled(ButtonWidth), 0.0f));
        ImGui::EndDisabled();

        if (canCreate && (createClicked || IsConfirmPressed()))
        {
            const ProjectResult result = CreateProject(PathFromUtf8(m_Location.data()), m_Name.data());
            if (result.IsValid())
            {
                m_LastLocation = result.Directory.parent_path();
                created = result.Directory;
                ImGui::CloseCurrentPopup();
            }
            else
            {
                m_NewProjectError = result.Error;
            }
        }
        return created;
    }

    std::optional<std::filesystem::path> ProjectPopups::ReceiveOpenedProject()
    {
        if (std::string error = m_ProjectDialog.TakeError(); !error.empty())
        {
            ShowOpenError(std::move(error));
            return std::nullopt;
        }

        const std::optional<std::filesystem::path> folder = m_ProjectDialog.TakeFolder();
        if(!folder)
        {
            return std::nullopt;
        }

        const ProjectResult result = OpenProject(*folder);
        if (!result.IsValid())
        {
            ShowOpenError(result.Error);
            return std::nullopt;
        }

        m_LastLocation = result.Directory.parent_path();
        return result.Directory;
    }

    void ProjectPopups::ShowOpenError(std::string error)
    {
        m_OpenError = std::move(error);
        ImGui::OpenPopup(OpenErrorPopupName);
    }

    void ProjectPopups::DrawOpenErrorPopup()
    {
        if (!BeginMessagePopup(OpenErrorPopupName))
        {
            return;
        }

        DrawWrappedText(ImGui::GetStyleColorVec4(ImGuiCol_Text), m_OpenError);
        DrawFooterSeparator();

        AlignButtonsRight(1);
        if (ImGui::Button("OK", ImVec2(Scaled(ButtonWidth), 0.0f)) || IsConfirmPressed() || IsCancelPressed())
        {
            ImGui::CloseCurrentPopup();
        }

        ImGui::EndPopup();
    }
}
