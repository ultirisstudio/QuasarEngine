#include <quasar/io/File.h>
#include <quasar/io/UserConfigDirectory.h>

#include <platform/SystemDirectories.h>

#include <filesystem>
#include <optional>

namespace Quasar
{
    std::optional<std::filesystem::path> GetUserConfigDirectory()
    {
        std::optional<std::filesystem::path> directory = Platform::GetSystemConfigDirectory();
        if (!directory)
        {
            return std::nullopt;
        }

        *directory /= "QuasarEngine";
        if (!CreateDirectories(*directory))
        {
            return std::nullopt;
        }

        return directory;
    }
}
