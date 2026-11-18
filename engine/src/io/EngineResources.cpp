#include <quasar/io/EngineResources.h>

#include <io/ExecutableDirectory.h>

#include <filesystem>
#include <optional>

namespace Quasar
{
    std::filesystem::path GetResourcesDirectory()
    {
        const std::optional<std::filesystem::path> directory = GetExecutableDirectory();
        if (!directory)
        {
            return {};
        }
        return *directory / "resources";
    }

    std::filesystem::path GetEngineResourcesDirectory()
    {
        const std::filesystem::path directory = GetResourcesDirectory();
        return directory.empty() ? directory : directory / "engine";
    }
}
