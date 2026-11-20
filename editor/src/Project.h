#pragma once

#include <filesystem>
#include <string>
#include <string_view>

namespace Quasar::Editor
{
    inline constexpr std::string_view AssetsDirectoryName = "Assets";

    inline constexpr std::string_view DefaultProjectsDirectoryName = "Quasar Projects";

    struct ProjectResult
    {
        std::filesystem::path Directory;
        std::string Error;

        [[nodiscard]] bool IsValid() const { return Error.empty(); }
    };

    [[nodiscard]] std::string PathToUtf8(const std::filesystem::path& path);

    [[nodiscard]] std::filesystem::path GetAssetsDirectory(const std::filesystem::path& projectDirectory);

    [[nodiscard]] std::filesystem::path ChooseDefaultProjectLocation(const std::filesystem::path& documents, const std::filesystem::path& home);

    [[nodiscard]] std::filesystem::path GetDefaultProjectLocation();

    [[nodiscard]] std::filesystem::path ResolveProjectDirectory(const std::filesystem::path& location, std::string_view name);

    [[nodiscard]] ProjectResult CreateProject(const std::filesystem::path& location, std::string_view name);

    [[nodiscard]] ProjectResult OpenProject(const std::filesystem::path& projectDirectory);
}
