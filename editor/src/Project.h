#pragma once

#include <filesystem>
#include <string>
#include <string_view>

namespace Quasar::Editor
{
    inline constexpr std::string_view AssetsDirectoryName = "Assets";

    inline constexpr std::string_view ProjectFileExtension = ".qproject";

    inline constexpr std::string_view DefaultProjectsDirectoryName = "Quasar Projects";

    // Version of the content of the .qproject file. Increase it when the content changes in a way that older editors
    // cannot read.
    inline constexpr int ProjectFormatVersion = 1;

    struct ProjectResult
    {
        std::filesystem::path Directory;
        std::filesystem::path ProjectFile;
        std::string Name;
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
