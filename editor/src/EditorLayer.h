#pragma once

/// \file
/// \brief The layer of the editor, which hosts the editor panels.
/// \author Kurogenshi
/// \date 2026-11-02

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
    /// @brief The layer of the editor, which owns the editor panels and the editor state they share.
    ///
    /// The panels are drawn in a dock space that covers the main window, under a menu bar whose View menu opens and
    /// closes them. They are updated, rendered and drawn in the order they were added.
    class EditorLayer final : public Layer
    {
    public:
        EditorLayer();

        /// @brief Adds the panels of the editor and logs that the editor started.
        void OnAttach() override;

        /// @brief Removes the panels and logs that the editor stopped.
        void OnDetach() override;

        /// @brief Updates every panel.
        void OnUpdate(Timestep timestep) override;

        /// @brief Lets every panel render, before the ImGui windows are drawn.
        void OnRender() override;

        /// @brief Draws the menu bar, the dock space over the main window, then every open panel.
        void OnImGuiRender() override;

        /// @brief Gives the event to the panels in their order, until one of them handles it.
        void OnEvent(Event& event) override;

        /// @brief Adds a panel after the others.
        /// @tparam T The class of the panel. Its constructor takes the editor context first, then the given arguments.
        /// @return The new panel, which the layer owns until it is detached.
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
