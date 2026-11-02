#pragma once

/// \file
/// \brief Creation of the editor application.
/// \author Kurogenshi
/// \date 2026-11-02

#include <quasar/core/Application.h>
#include <quasar/core/Base.h>

#include <string>
#include <vector>

/// @brief The editor of QuasarEngine. The engine never depends on this code.
namespace Quasar::Editor
{
    /// @brief Returns the settings of the editor application and of its main window.
    /// @param arguments The command-line arguments, without the name of the program.
    /// @return The settings, with the arguments.
    ApplicationSpecification GetEditorSpecification(std::vector<std::string> arguments);

    /// @brief Creates the editor application, with the editor layer on its layer stack.
    /// @param arguments The command-line arguments, without the name of the program.
    /// @return The editor application, or `nullptr` if it cannot be created. The reason is logged as an engine error.
    [[nodiscard]] Scope<Application> CreateEditorApplication(std::vector<std::string> arguments);
}