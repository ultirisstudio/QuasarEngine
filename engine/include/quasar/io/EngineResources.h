#pragma once

/// \file
/// \brief The folder of the resources that every application of the engine needs, such as its shaders and fonts.
/// \author Kurogenshi
/// \date 2026-11-18

#include <filesystem>

namespace Quasar
{
    /// @brief Returns the folder of the engine resources: `resources/engine`, in the folder of the executable.
    ///
    /// The build copies `engine/resources` there. The resources of an application, such as the sandbox or the editor,
    /// stay in their own folder.
    /// @return The folder, or an empty path if the folder of the executable cannot be found.
    std::filesystem::path GetEngineResourcesDirectory();
}
