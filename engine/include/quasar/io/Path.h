#pragma once

/// \file
/// \brief Functions to convert paths to the form stored in engine files.
/// \author Kurogenshi
/// \date 2026-10-25

#include <filesystem>
#include <optional>
#include <string>

namespace Quasar
{
    /// @brief Converts a path to the form stored in engine files, the same on every platform.
    ///
    /// Both `/` and `\` are read as separators and the result always uses `/`. Redundant parts are removed: `.`,
    /// `..` after a directory name, repeated separators and the separator at the end.
    ///
    /// The case of the letters is kept: on Linux, `Assets` and `assets` are different directories.
    /// @param path The path to convert. It is not read from the disk, so the file does not need to exist.
    /// @return The normalized path, or an empty string for an empty path.
    std::string NormalizePath(const std::filesystem::path& path);

    /// @brief Makes a path relative to a root folder, in the form stored in engine files.
    ///
    /// Relative paths are first made absolute from the current directory. The paths are not read from the disk.
    /// Nothing is logged when the path is outside the root: the caller knows what the path is for, so it can report a
    /// clearer error.
    /// @param path The path to convert, for example `C:/Projects/Game/Assets/cube.obj`.
    /// @param root The root folder, for example `C:/Projects/Game`.
    /// @return The normalized relative path, for example `Assets/cube.obj`, `.` for the root itself, or no value if
    /// the path is not inside the root.
    std::optional<std::string> MakeRelativePath(const std::filesystem::path& path, const std::filesystem::path& root);
}
