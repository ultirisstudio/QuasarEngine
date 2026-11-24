#include <quasar/io/UserFolders.h>

#include <quasar/io/Path.h>
#include <quasar/log/Log.h>

#include <SDL3/SDL_error.h>
#include <SDL3/SDL_filesystem.h>

namespace Quasar
{
    std::optional<std::filesystem::path> GetUserFolder(UserFolder folder)
    {
        const char* path = SDL_GetUserFolder(folder == UserFolder::Home ? SDL_FOLDER_HOME : SDL_FOLDER_DOCUMENTS);
        if (path == nullptr || *path == '\0')
        {
            if (folder == UserFolder::Home)
            {
                QUASAR_ENGINE_ERROR("Cannot find the home folder of the user: {}", SDL_GetError());
            }
            return std::nullopt;
        }

        std::filesystem::path directory = PathFromUtf8(path).lexically_normal();
        if (!directory.has_filename())
        {
            directory = directory.parent_path();
        }
        return directory;
    }
}
