#include "Project.h"

#include <quasar/io/Path.h>
#include <quasar/io/UserFolders.h>

#include <algorithm>
#include <array>
#include <cctype>
#include <cerrno>
#include <fstream>
#include <optional>
#include <system_error>
#include <utility>

namespace Quasar::Editor
{
    namespace
    {
        constexpr std::string_view ForbiddenCharacters = "<>:\"/\\|?*";
        constexpr unsigned char FirstPrintableCharacter = 0x20U;
        constexpr std::array<std::string_view, 22> ReservedNames = {"CON",  "PRN",  "AUX",  "NUL",  "COM1", "COM2", "COM3", "COM4", "COM5", "COM6", "COM7",
                                                                    "COM8", "COM9", "LPT1", "LPT2", "LPT3", "LPT4", "LPT5", "LPT6", "LPT7", "LPT8", "LPT9"};

        bool IsReservedName(std::string_view name)
        {
            const std::string_view stem = name.substr(0, name.find('.'));
            return std::ranges::any_of(ReservedNames,
                                       [stem](std::string_view reserved)
                                       {
                                           return std::ranges::equal(
                                               stem, reserved, [](char left, char right)
                                               { return std::toupper(static_cast<unsigned char>(left)) == static_cast<unsigned char>(right); });
                                       });
        }

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
            if (name.find_first_of(ForbiddenCharacters) != std::string_view::npos || IsReservedName(name))
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
            return std::ranges::any_of(path,
                                       [](const std::filesystem::path& part)
                                       {
                                           const std::u8string name = part.u8string();
                                           return name == u8"OneDrive" || name.starts_with(u8"OneDrive - ");
                                       });
        }

        std::filesystem::file_type GetFileType(const std::filesystem::path& path, std::error_code& error)
        {
            const std::filesystem::file_type type = std::filesystem::status(path, error).type();
            if (type == std::filesystem::file_type::not_found)
            {
                error.clear();
            }
            return type;
        }

        std::optional<std::filesystem::path> FindProjectFile(const std::filesystem::path& projectDirectory, std::error_code& error)
        {
            for (const std::filesystem::directory_entry& entry : std::filesystem::directory_iterator(projectDirectory, error))
            {
                if (entry.path().extension() != ProjectFileExtension)
                {
                    continue;
                }
                const std::filesystem::file_type type = GetFileType(entry.path(), error);
                if (error)
                {
                    return std::nullopt;
                }
                if (type == std::filesystem::file_type::regular)
                {
                    return entry.path();
                }
            }
            return std::nullopt;
        }

        std::string GetWriteError()
        {
            return errno != 0 ? std::generic_category().message(errno) : std::string("the file cannot be written");
        }

        std::string WriteEmptyFile(const std::filesystem::path& path)
        {
            errno = 0;
            std::ofstream stream(path, std::ios::binary);
            if (!stream)
            {
                return GetWriteError();
            }
            stream.close();
            if (!stream)
            {
                return GetWriteError();
            }
            return {};
        }

        ProjectResult FailAndRemove(const std::filesystem::path& directory, std::string error)
        {
            std::error_code ignored;
            std::filesystem::remove_all(directory, ignored);
            return Fail(std::move(error));
        }
    }

    std::filesystem::path GetAssetsDirectory(const std::filesystem::path& projectDirectory)
    {
        return projectDirectory / AssetsDirectoryName;
    }

    std::filesystem::path GetProjectFile(const std::filesystem::path& projectDirectory)
    {
        const std::filesystem::path directory = projectDirectory.has_filename() ? projectDirectory : projectDirectory.parent_path();
        std::filesystem::path file = directory / directory.filename();
        file += ProjectFileExtension;
        return file;
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
        return ChooseDefaultProjectLocation(GetUserFolder(UserFolder::Documents).value_or(std::filesystem::path()),
                                            GetUserFolder(UserFolder::Home).value_or(std::filesystem::path()));
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
        const bool exists = std::filesystem::exists(directory, error);
        if (error)
        {
            return Fail("Cannot check the folder " + Quote(directory) + ": " + error.message());
        }
        if (exists)
        {
            return Fail("The folder " + Quote(directory) + " already exists");
        }

        const std::filesystem::path assets = GetAssetsDirectory(directory);
        std::filesystem::create_directories(assets, error);
        if (error)
        {
            return FailAndRemove(directory, "Cannot create the folder " + Quote(assets) + ": " + error.message());
        }

        const std::filesystem::path projectFile = GetProjectFile(directory);
        if (const std::string reason = WriteEmptyFile(projectFile); !reason.empty())
        {
            return FailAndRemove(directory, "Cannot create the file " + Quote(projectFile) + ": " + reason);
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
        std::filesystem::path directory = std::filesystem::absolute(projectDirectory, error).lexically_normal();
        if (error)
        {
            return Fail("Cannot find the folder " + Quote(projectDirectory) + ": " + error.message());
        }
        if (!directory.has_filename())
        {
            directory = directory.parent_path();
        }

        const std::filesystem::file_type type = GetFileType(directory, error);
        if (error)
        {
            return Fail("Cannot read the folder " + Quote(directory) + ": " + error.message());
        }
        if (type == std::filesystem::file_type::not_found)
        {
            return Fail("The folder " + Quote(directory) + " does not exist");
        }
        if (type != std::filesystem::file_type::directory)
        {
            return Fail(Quote(directory) + " is not a folder");
        }

        const std::optional<std::filesystem::path> projectFile = FindProjectFile(directory, error);
        if (error)
        {
            return Fail("Cannot read the folder " + Quote(directory) + ": " + error.message());
        }
        if (!projectFile)
        {
            return Fail("The folder " + Quote(directory) + " is not a project: it has no " + std::string(ProjectFileExtension) + " file");
        }

        const bool hasAssets = GetFileType(GetAssetsDirectory(directory), error) == std::filesystem::file_type::directory;
        if (error)
        {
            return Fail("Cannot read the folder " + Quote(GetAssetsDirectory(directory)) + ": " + error.message());
        }
        if (!hasAssets)
        {
            return Fail("The project " + Quote(directory) + " has no " + std::string(AssetsDirectoryName) + " folder");
        }

        return {.Directory = std::move(directory), .Error = {}};
    }
}
