#include <quasar/io/EngineResources.h>

#include <io/ExecutableDirectory.h>

#include <filesystem>
#include <optional>

namespace Quasar
{
    std::filesystem::path GetEngineResourcesDirectory()
    {
        const std::optional<std::filesystem::path> directory = GetExecutableDirectory();
        if (!directory)
        {
            return {};
        }
        return *directory / "resources" / "engine";
    }
}
