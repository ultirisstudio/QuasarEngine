#pragma once

#include "EditorContext.h"
#include "EditorPanel.h"
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
        void DrawMenuBar();
        void SetProject(const std::filesystem::path& directory);

        EditorContext m_Context;
        std::vector<Scope<EditorPanel>> m_Panels;
        ProjectPopups m_ProjectPopups;
    };
}
