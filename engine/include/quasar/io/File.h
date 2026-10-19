#pragma once

/// \file
/// \brief Functions to read a whole file from the disk.
/// \author Kurogenshi
/// \date 2026-10-19

#include <cstddef>
#include <filesystem>
#include <optional>
#include <string>
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
}
