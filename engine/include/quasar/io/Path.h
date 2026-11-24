#pragma once

/// \file
/// \brief Functions to convert paths to the form stored in engine files.
/// \author Kurogenshi
/// \date 2026-10-25

#include <filesystem>
#include <optional>
#include <string>
#include <string_view>

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

    /// @brief Creates a path from UTF-8 text, such as the paths given by SDL or read from engine files.
    ///
    /// On Windows, `std::filesystem::path` reads narrow text in the code page of the system, not in UTF-8, so a path
    /// with characters outside this code page would be wrong. This function reads the text as UTF-8 on every platform.
    /// @param text The path, in UTF-8.
    /// @return The path.
    std::filesystem::path PathFromUtf8(std::string_view text);

    /// @brief Gives the UTF-8 text of a path, such as the text shown by ImGui, written in the logs or given to SDL.
    ///
    /// On Windows, `std::filesystem::path::string()` writes the text in the code page of the system, not in UTF-8, so a
    /// path with characters outside this code page would be wrong. This function writes UTF-8 on every platform, and is
    /// the opposite of PathFromUtf8().
    /// @param path The path.
    /// @return The path in UTF-8, with the separators of the platform.
    std::string PathToUtf8(const std::filesystem::path& path);

    /// @brief Tells if a text can be the name of a file or a folder on every platform that the engine supports.
    ///
    /// A name is refused if it is empty, `.` or `..`, ends with a space or a dot, contains one of `< > : " / \ | ? *`
    /// or a control character, or is a name that Windows keeps for its devices, such as `CON`, `NUL`, `COM1` or `LPT9`,
    /// in any case and even with an extension. Other UTF-8 characters are allowed.
    /// @param name The name, in UTF-8, without a folder.
    bool IsValidFileName(std::string_view name);

    /// @brief Returns a folder that exists: the path itself if it is a folder, or its closest parent that is one.
    ///
    /// For example to open a dialog on the folder that a user is typing, before it exists.
    /// @param path The path, which does not need to exist.
    /// @return The folder, or an empty path if no part of the path is an existing folder.
    std::filesystem::path FindExistingDirectory(std::filesystem::path path);
}
