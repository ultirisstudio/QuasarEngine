#pragma once

#include <cstddef>
#include <filesystem>
#include <optional>
#include <string>
#include <vector>

namespace Quasar
{
    std::optional<std::string> ReadTextFile(const std::filesystem::path& path);

    std::optional<std::vector<std::byte>> ReadBinaryFile(const std::filesystem::path& path);
}
