#pragma once

#include "EditorContext.h"
#include "EditorPanel.h"
#include "EditorSettings.h"
#include "ProjectPopups.h"

#include <quasar/core/Base.h>
#include <quasar/core/Layer.h>
#include <quasar/time/Timestep.h>

#include <concepts>
#include <filesystem>
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
        // The recent projects and the last location are saved in this file. An empty path keeps them in memory only.
        explicit EditorLayer(std::filesystem::path settingsFile = {});

        const EditorContext& GetContext() const { return m_Context; }

        const EditorSettings& GetSettings() const { return m_Settings; }

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
        void DrawMenuBar();
        void DrawRecentProjectsMenu();
        void DrawLanguageMenu();
        void SetProject(const ProjectResult& project);

        EditorContext m_Context;
        std::vector<Scope<EditorPanel>> m_Panels;
        EditorSettings m_Settings;
        ProjectPopups m_ProjectPopups;
    };
}
