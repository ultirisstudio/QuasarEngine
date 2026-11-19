#pragma once

#include "EditorContext.h"
#include "panels/EditorPanel.h"
#include "Project.h"

#include <quasar/core/Base.h>
#include <quasar/core/Layer.h>
#include <quasar/time/Timestep.h>

#include <array>
#include <concepts>
#include <cstdint>
#include <filesystem>
#include <string>
#include <utility>
#include <vector>

namespace Quasar
{
    class Event;
}

namespace Quasar::Editor
{
    class EditorLayer final : public Layer
    {
    public:
        EditorLayer();

        void OnAttach() override;

        void OnDetach() override;

        void OnUpdate(Timestep timestep) override;

        void OnRender() override;

        void OnImGuiRender() override;

        void OnEvent(Event& event) override;

        template <std::derived_from<EditorPanel> T, typename... Args>
        T& AddPanel(Args&&... args)
        {
            Scope<T> panel = CreateScope<T>(m_Context, std::forward<Args>(args)...);
            T& added = *panel;
            m_Panels.push_back(std::move(panel));
            return added;
        }

    private:
        enum class ProjectPopup : std::uint8_t
        {
            None,
            NewProject,
            OpenProject
        };

        using TextBuffer = std::array<char, 512>;

        void DrawMenuBar();

        void RequestProjectPopup(ProjectPopup popup);
        void DrawProjectPopups();
        void DrawNewProjectPopup();
        void DrawOpenProjectPopup();
        void DrawProjectError() const;
        void ApplyProjectResult(const ProjectResult& result);
        void SetProject(const std::filesystem::path& directory);

        EditorContext m_Context;
        std::vector<Scope<EditorPanel>> m_Panels;

        ProjectPopup m_RequestedPopup = ProjectPopup::None;
        TextBuffer m_ProjectName{};
        TextBuffer m_ProjectLocation{};
        std::string m_ProjectError;
    };
}
