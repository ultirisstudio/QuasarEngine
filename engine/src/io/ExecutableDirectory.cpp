#include <io/ExecutableDirectory.h>

#include <quasar/io/Path.h>
#include <quasar/log/Log.h>

#include <SDL3/SDL_error.h>
#include <SDL3/SDL_filesystem.h>

#include <filesystem>
#include <optional>

namespace Quasar
{
    std::optional<std::filesystem::path> GetExecutableDirectory()
    {
        const char* directory = SDL_GetBasePath();
        if (directory == nullptr)
        {
            QUASAR_ENGINE_ERROR("Cannot find the folder of the executable: {}", SDL_GetError());
            return std::nullopt;
        }

        return PathFromUtf8(directory).parent_path();
    }
}
