#pragma once

#include "EditorContext.h"
#include "panels/EditorPanel.h"

#include <quasar/core/Base.h>
#include <quasar/core/Layer.h>
#include <quasar/time/Timestep.h>

#include <concepts>
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

        EditorContext m_Context;
        std::vector<Scope<EditorPanel>> m_Panels;
    };
}
