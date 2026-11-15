#pragma once

#include <filesystem>

namespace Quasar::Editor
{
    struct EditorContext
    {
        std::filesystem::path ProjectDirectory;
    };
}
