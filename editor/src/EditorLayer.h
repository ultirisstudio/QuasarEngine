#pragma once

/// \file
/// \brief The layer of the editor, which hosts the editor panels.
/// \author Kurogenshi
/// \date 2026-11-02

#include <quasar/core/Layer.h>
#include "panels/ConsolePanel.h"

namespace Quasar::Editor
{
    /// @brief The layer of the editor. It will host every editor panel, and the editor state they share.
    class EditorLayer final : public Layer
    {
    public:
        EditorLayer();

        /// @brief Logs that the editor started.
        void OnAttach() override;

        /// @brief Logs that the editor stopped.
        void OnDetach() override;
        
        /// @brief Draws the editor panels.
        void OnImGuiRender() override;

    private:
       ConsolePanel m_Console;
    };
}
