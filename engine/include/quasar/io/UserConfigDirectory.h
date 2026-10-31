#pragma once

/// \file
/// \brief The folder where the engine stores the settings of the current user.
/// \author Kurogenshi
/// \date 2026-10-31

#include <filesystem>
#include <optional>

namespace Quasar
{
    /// @brief Returns the folder where the engine stores the settings of the current user, such as the list of recent
    /// projects. The folder is created if it does not exist yet.
    ///
    /// The folder is named QuasarEngine and is placed in the folder that the system gives for application settings:
    /// - on Windows, the roaming application data folder, usually `%APPDATA%`
    /// - on Linux, `$XDG_CONFIG_HOME` when it is an absolute path, otherwise `~/.config`
    ///
    /// @return The absolute path of the folder, or no value if it cannot be found or created. The reason is logged as
    /// an engine error.
    std::optional<std::filesystem::path> GetUserConfigDirectory();
}