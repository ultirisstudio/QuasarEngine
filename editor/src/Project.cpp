#include "Project.h"

#include <quasar/io/File.h>
#include <quasar/io/Path.h>

#include <SDL3/SDL_filesystem.h>

#include <algorithm>
#include <system_error>
#include <utility>

namespace Quasar::Editor
{
    namespace
    {
        constexpr std::string_view ForbiddenCharacters = "<>:\"/\\|?*";
        constexpr unsigned char FirstPrintableCharacter = 0x20U;

        bool IsValidProjectName(std::string_view name)
        {
            if (name.empty() || name == "." || name == "..")
            {
                return false;
            }
            if (name.back() == ' ' || name.back() == '.')
            {
                return false;
            }
            if (name.find_first_of(ForbiddenCharacters) != std::string_view::npos)
            {
                return false;
            }
            return std::ranges::none_of(name, [](char character) { return static_cast<unsigned char>(character) < FirstPrintableCharacter; });
        }

        ProjectResult Fail(std::string error)
        {
            return {.Directory = {}, .Error = std::move(error)};
        }

        std::string Quote(const std::filesystem::path& path)
        {
            return "\"" + PathToUtf8(path) + "\"";
        }

        bool IsInsideOneDrive(const std::filesystem::path& path)
        {
            constexpr std::u8string_view oneDriveName = u8"OneDrive";
            return std::ranges::any_of(path, [oneDriveName](const std::filesystem::path& part) { return part.u8string().starts_with(oneDriveName); });
        }

        std::filesystem::path GetUserFolder(SDL_Folder folder)
        {
            const char* path = SDL_GetUserFolder(folder);
            return path != nullptr ? PathFromUtf8(path) : std::filesystem::path();
        }
    }

    std::string PathToUtf8(const std::filesystem::path& path)
    {
        const std::u8string utf8 = path.u8string();
        std::string text(utf8.size(), '\0');
        std::ranges::transform(utf8, text.begin(), [](char8_t character) { return static_cast<char>(character); });
        return text;
    }

    std::filesystem::path GetAssetsDirectory(const std::filesystem::path& projectDirectory)
    {
        return projectDirectory / AssetsDirectoryName;
    }

    std::filesystem::path ChooseDefaultProjectLocation(const std::filesystem::path& documents, const std::filesystem::path& home)
    {
        std::filesystem::path folder;
        if (!documents.empty() && (home.empty() || !IsInsideOneDrive(documents)))
        {
            folder = documents;
        }
        else if (!home.empty())
        {
            folder = documents.empty() ? home : home / "Documents";
        }
        else
        {
            return {};
        }
        return (folder / DefaultProjectsDirectoryName).lexically_normal();
    }

    std::filesystem::path GetDefaultProjectLocation()
    {
        return ChooseDefaultProjectLocation(GetUserFolder(SDL_FOLDER_DOCUMENTS), GetUserFolder(SDL_FOLDER_HOME));
    }

    std::filesystem::path ResolveProjectDirectory(const std::filesystem::path& location, std::string_view name)
    {
        if (!location.is_absolute() || !IsValidProjectName(name))
        {
            return {};
        }
        return (location / PathFromUtf8(name)).lexically_normal();
    }

    ProjectResult CreateProject(const std::filesystem::path& location, std::string_view name)
    {
        if (!IsValidProjectName(name))
        {
            return Fail("\"" + std::string(name) + "\" is not a valid project name");
        }
        if (location.empty())
        {
            return Fail("Choose the folder in which the project is created");
        }
        if (!location.is_absolute())
        {
            return Fail("The location " + Quote(location) + " must be a full path, starting from the root of the disk");
        }

        const std::filesystem::path directory = ResolveProjectDirectory(location, name);
        std::error_code error;
        if (std::filesystem::exists(directory, error))
        {
            return Fail("The folder " + Quote(directory) + " already exists");
        }

        if (!CreateDirectories(GetAssetsDirectory(directory)))
        {
            return Fail("Cannot create the folder " + Quote(directory) + ": see the console for the reason");
        }

        return {.Directory = directory, .Error = {}};
    }

    ProjectResult OpenProject(const std::filesystem::path& projectDirectory)
    {
        if (projectDirectory.empty())
        {
            return Fail("Choose the folder of the project");
        }

        std::error_code error;
        if (!std::filesystem::is_directory(projectDirectory, error))
        {
            return Fail("The folder " + Quote(projectDirectory) + " does not exist");
        }
        if (!std::filesystem::is_directory(GetAssetsDirectory(projectDirectory), error))
        {
            return Fail("The folder " + Quote(projectDirectory) + " is not a project: it has no " + std::string(AssetsDirectoryName) + " folder");
        }

        std::filesystem::path directory = std::filesystem::absolute(projectDirectory, error).lexically_normal();
        if (error)
        {
            return Fail("Cannot resolve the folder of the project: " + error.message());
        }
        if (!directory.has_filename())
        {
            directory = directory.parent_path();
        }
        return {.Directory = std::move(directory), .Error = {}};
    }
}
