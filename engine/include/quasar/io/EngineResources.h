#pragma once

/// \file
/// \brief The folders of the resources: the ones of the engine, such as its shaders and fonts, and the ones of each
/// application.
/// \author Kurogenshi
/// \date 2026-11-18

#include <filesystem>

namespace Quasar
{
    /// @brief Returns the folder of the resources: `resources`, in the folder of the executable.
    ///
    /// Each application copies its own resources into a folder of its name there, such as `resources/sandbox`.
    /// @return The folder, or an empty path if the folder of the executable cannot be found.
    std::filesystem::path GetResourcesDirectory();

    /// @brief Returns the folder of the engine resources: `resources/engine`, in the folder of the executable.
    ///
    /// The build copies `engine/resources` there. The resources of an application, such as the sandbox or the editor,
    /// stay in their own folder.
    /// @return The folder, or an empty path if the folder of the executable cannot be found.
    std::filesystem::path GetEngineResourcesDirectory();
}
