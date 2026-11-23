#pragma once

#include <filesystem>
#include <string>
#include <string_view>

namespace Quasar::Editor
{
    inline constexpr std::string_view AssetsDirectoryName = "Assets";

    inline constexpr std::string_view ProjectFileExtension = ".qproject";

    inline constexpr std::string_view DefaultProjectsDirectoryName = "Quasar Projects";

    struct ProjectResult
    {
        std::filesystem::path Directory;
        std::string Error;

        bool IsValid() const { return Error.empty(); }
    };

    std::filesystem::path GetAssetsDirectory(const std::filesystem::path& projectDirectory);

    std::filesystem::path GetProjectFile(const std::filesystem::path& projectDirectory);

    std::filesystem::path ChooseDefaultProjectLocation(const std::filesystem::path& documents, const std::filesystem::path& home);

    std::filesystem::path GetDefaultProjectLocation();

    std::filesystem::path ResolveProjectDirectory(const std::filesystem::path& location, std::string_view name);

    ProjectResult CreateProject(const std::filesystem::path& location, std::string_view name);

    ProjectResult OpenProject(const std::filesystem::path& projectDirectory);
}
