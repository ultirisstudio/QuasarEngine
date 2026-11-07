#pragma once

/// \file
/// \brief The folder where an application stores the files of the current user.
/// \author Kurogenshi
/// \date 2026-11-07

#include <filesystem>
#include <optional>
#include <string_view>

namespace Quasar
{
    /// @brief Tells if a name can be used as the organization or the application name of a user data folder.
    ///
    /// The name becomes a folder name, so it must not be empty, must not start or end with a space, and must only
    /// contain letters, digits and spaces. Letters outside of ASCII are allowed when they are written in UTF-8.
    /// @param name The name to check, in UTF-8.
    /// @return True if the name can be used.
    bool IsValidUserDataName(std::string_view name);

    /// @brief Returns the folder where an application stores the files of the current user, such as its settings or
    /// its saved games. The folder is created if it does not exist yet.
    ///
    /// SDL finds the folder that the system gives for application data, and the folder is placed in it:
    /// - on Windows, `%APPDATA%\<organization>\<application>`
    /// - on Linux, `$XDG_DATA_HOME/<organization>/<application>`, usually `~/.local/share/<organization>/<application>`
    ///
    /// Use the same organization for all the applications of a studio, and never change the name of an application
    /// once it is released, or its users lose their files. Application::GetUserDataDirectory() gives the folder of the
    /// running application.
    /// @param organization The name of the studio, in UTF-8. See IsValidUserDataName().
    /// @param application The name of the application, in UTF-8. See IsValidUserDataName().
    /// @return The absolute path of the folder, or no value if a name is not valid or if the folder cannot be found or
    /// created. The reason is logged as an engine error.
    std::optional<std::filesystem::path> GetUserDataDirectory(std::string_view organization, std::string_view application);
}
