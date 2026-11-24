#pragma once

/// \file
/// \brief The folders of the current user that the system knows, such as the documents.
/// \author Fabien Wahbeh
/// \date 2026-11-23

#include <cstdint>
#include <filesystem>
#include <optional>

namespace Quasar
{
    /// @brief A folder of the current user that the system knows, as found by GetUserFolder().
    enum class UserFolder : std::uint8_t
    {
        Home,     ///< The folder of the user, such as `C:\Users\<user>` on Windows or `$HOME` on Linux.
        Documents ///< The documents of the user. On Windows, OneDrive can move it into its own folder.
    };

    /// @brief Returns a folder of the current user, as the system gives it. The folder is not created.
    ///
    /// On Linux, the documents folder comes from `~/.config/user-dirs.dirs`, so a system without this file has no
    /// documents folder. This is a normal case, which is not logged.
    /// @param folder The folder to find.
    /// @return The absolute path of the folder, or no value if the system does not give it. A missing home folder is
    /// logged as an engine error.
    std::optional<std::filesystem::path> GetUserFolder(UserFolder folder);
}
