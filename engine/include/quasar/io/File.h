#pragma once

/// \file
/// \brief Functions to read and write whole files, and to create directories.
/// \author Kurogenshi
/// \date 2026-10-19

#include <cstddef>
#include <filesystem>
#include <optional>
#include <span>
#include <string>
#include <string_view>
#include <vector>

namespace Quasar
{
    /// @brief Reads a whole text file.
    ///
    /// The content is returned exactly as stored, so line endings are the same on every platform. A UTF-8 byte
    /// order mark at the start of the file is removed. An empty file gives an empty string.
    /// @param path Path of the file to read.
    /// @return The content of the file, or no value if the file is missing, is a folder or cannot be read. The
    /// reason is logged as an engine error.
    std::optional<std::string> ReadTextFile(const std::filesystem::path& path);

    /// @brief Reads a whole binary file, byte for byte.
    ///
    /// An empty file gives an empty vector.
    /// @param path Path of the file to read.
    /// @return The bytes of the file, or no value if the file is missing, is a folder or cannot be read. The
    /// reason is logged as an engine error.
    std::optional<std::vector<std::byte>> ReadBinaryFile(const std::filesystem::path& path);

    /// @brief Writes a whole text file, replacing its content if it already exists.
    ///
    /// The text is written exactly as given, so line endings are the same on every platform. The missing parent
    /// directories are created.
    /// @param path Path of the file to write.
    /// @param text Content of the file.
    /// @return True if the file was written. Otherwise the reason is logged as an engine error.
    bool WriteTextFile(const std::filesystem::path& path, std::string_view text);

    /// @brief Writes a whole binary file, byte for byte, replacing its content if it already exists.
    ///
    /// The missing parent directories are created.
    /// @param path Path of the file to write.
    /// @param data Content of the file.
    /// @return True if the file was written. Otherwise the reason is logged as an engine error.
    bool WriteBinaryFile(const std::filesystem::path& path, std::span<const std::byte> data);

    /// @brief Creates a directory and its missing parents.
    /// @param path Path of the directory to create.
    /// @return True if the directory exists after the call, even if it already existed. Otherwise the reason is
    /// logged as an engine error.
    bool CreateDirectories(const std::filesystem::path& path);
}
