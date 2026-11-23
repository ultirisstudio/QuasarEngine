#include <quasar/io/UserDataDirectory.h>

#include <quasar/io/Path.h>
#include <quasar/log/Log.h>

#include <SDL3/SDL_error.h>
#include <SDL3/SDL_filesystem.h>
#include <SDL3/SDL_stdinc.h>

#include <algorithm>
#include <filesystem>
#include <memory>
#include <optional>
#include <string>
#include <string_view>

namespace Quasar
{
    namespace
    {
        struct SdlFree
        {
            void operator()(char* text) const { SDL_free(text); }
        };

        bool IsAllowedCharacter(char character)
        {
            const auto byte = static_cast<unsigned char>(character);

            // Bytes from 0x80 are the parts of UTF-8 characters outside of ASCII
            constexpr unsigned char firstNonAsciiByte = 0x80;
            if (byte >= firstNonAsciiByte)
            {
                return true;
            }

            return (byte >= 'a' && byte <= 'z') || (byte >= 'A' && byte <= 'Z') || (byte >= '0' && byte <= '9') || byte == ' ';
        }
    }

    bool IsValidUserDataName(std::string_view name)
    {
        if (name.empty() || name.front() == ' ' || name.back() == ' ')
        {
            return false;
        }

        return std::ranges::all_of(name, IsAllowedCharacter);
    }

    std::optional<std::filesystem::path> GetUserDataDirectory(std::string_view organization, std::string_view application)
    {
        if (!IsValidUserDataName(organization) || !IsValidUserDataName(application))
        {
            QUASAR_ENGINE_ERROR("Cannot use '{}' and '{}' as the names of a user data folder: use only letters, digits and spaces", organization, application);
            return std::nullopt;
        }

        // SDL needs texts that end with a null character
        const std::string organizationText(organization);
        const std::string applicationText(application);

        // SDL creates the folder and gives it in UTF-8, with a separator at the end. The text must be freed.
        const std::unique_ptr<char, SdlFree> directory(SDL_GetPrefPath(organizationText.c_str(), applicationText.c_str()));
        if (directory == nullptr)
        {
            QUASAR_ENGINE_ERROR("Cannot find or create the user data folder of '{}': {}", application, SDL_GetError());
            return std::nullopt;
        }

        return PathFromUtf8(directory.get()).parent_path();
    }

    std::optional<std::filesystem::path> GetUserFolder(UserFolder folder)
    {
        const SDL_Folder sdlFolder = folder == UserFolder::Home ? SDL_FOLDER_HOME : SDL_FOLDER_DOCUMENTS;
        const char* name = folder == UserFolder::Home ? "home" : "documents";
        const char* path = SDL_GetUserFolder(sdlFolder);
        if (path == nullptr || *path == '\0')
        {
            QUASAR_ENGINE_ERROR("Cannot find the {} folder of the user: {}", name, SDL_GetError());
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
