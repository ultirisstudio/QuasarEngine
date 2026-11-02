#pragma once

#include <filesystem>
#include <optional>

namespace Quasar::Platform
{
    std::optional<std::filesystem::path> GetSystemConfigDirectory();

    std::optional<std::filesystem::path> GetExecutableDirectory();
}
