#include "Project.h"

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
    }

    std::filesystem::path PathFromUtf8(std::string_view text)
    {
        std::u8string utf8;
        utf8.reserve(text.size());
        for (const char character : text)
        {
            utf8.push_back(static_cast<char8_t>(character));
        }
        return {utf8};
    }

    std::string PathToUtf8(const std::filesystem::path& path)
    {
        const std::u8string utf8 = path.u8string();
        std::string text;
        text.reserve(utf8.size());
        for (const char8_t character : utf8)
        {
            text.push_back(static_cast<char>(character));
        }
        return text;
    }

    std::filesystem::path GetAssetsDirectory(const std::filesystem::path& projectDirectory)
    {
        return projectDirectory / AssetsDirectoryName;
    }

    std::filesystem::path ResolveProjectDirectory(const std::filesystem::path& parentDirectory, std::string_view name)
    {
        if (!parentDirectory.is_absolute() || !IsValidProjectName(name))
        {
            return {};
        }
        return (parentDirectory / PathFromUtf8(name)).lexically_normal();
    }

    ProjectResult CreateProject(const std::filesystem::path& parentDirectory, std::string_view name)
    {
        if (!IsValidProjectName(name))
        {
            return Fail("\"" + std::string(name) + "\" is not a valid project name");
        }
        if (parentDirectory.empty())
        {
            return Fail("Choose the folder in which the project is created");
        }
        if (!parentDirectory.is_absolute())
        {
            return Fail("The location " + Quote(parentDirectory) + " must be a full path, starting from the root of the disk");
        }

        std::error_code error;
        if (!std::filesystem::is_directory(parentDirectory, error))
        {
            return Fail("The folder " + Quote(parentDirectory) + " does not exist");
        }

        const std::filesystem::path directory = ResolveProjectDirectory(parentDirectory, name);
        if (std::filesystem::exists(directory, error))
        {
            return Fail("The folder " + Quote(directory) + " already exists");
        }
        if (!std::filesystem::create_directory(directory, error))
        {
            return Fail("Cannot create the folder " + Quote(directory) + ": " + error.message());
        }
        if(!std::filesystem::create_directory(GetAssetsDirectory(directory), error))
        {
            return Fail("Cannot create the folder " + Quote(GetAssetsDirectory(directory)) + ": " + error.message());
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

        const std::filesystem::path directory = std::filesystem::absolute(projectDirectory, error);
        if (error)
        {
            return Fail("Cannot resolve the folder of the project: " + error.message());
        }
        return {.Directory = directory.lexically_normal(), .Error = {}};
    }
}
