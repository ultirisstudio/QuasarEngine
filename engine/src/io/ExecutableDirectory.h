#pragma once

#include <filesystem>
#include <optional>

namespace Quasar
{
    std::optional<std::filesystem::path> GetExecutableDirectory();
}
