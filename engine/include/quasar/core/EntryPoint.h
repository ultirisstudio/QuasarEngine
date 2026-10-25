#pragma once

/// \file
/// \brief The function that each program built on the engine provides, to create its application.
/// \author Kurogenshi
/// \date 2026-10-25

#include <quasar/core/Application.h>
#include <quasar/core/Base.h>

#include <string>
#include <vector>

namespace Quasar
{
    /// @brief Creates the application of the program. Each program built on the engine defines this function.
    ///
    /// The engine defines `main()` in the Quasar::EntryPoint library, which the program links to. `main()` starts the
    /// logging, calls this function, runs the application, destroys it, then stops the logging. The program never
    /// defines `main()` itself:
    ///
    /// ```cpp
    /// Quasar::Scope<Quasar::Application> Quasar::CreateApplication(std::vector<std::string> arguments)
    /// {
    ///     return Application::Create({.Name = "Sandbox", .Arguments = std::move(arguments)});
    /// }
    /// ```
    ///
    /// @param arguments The command-line arguments, without the name of the program.
    /// @return The application to run, or `nullptr` if it cannot be created. The program then exits with code 1.
    Scope<Application> CreateApplication(std::vector<std::string> arguments);
}