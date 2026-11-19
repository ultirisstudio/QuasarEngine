#pragma once

#include <filesystem>
#include <string>
#include <string_view>

namespace Quasar::Editor
{
    inline constexpr std::string_view AssetsDirectoryName = "Assets";

    struct ProjectResult
    {
        std::filesystem::path Directory;
        std::string Error;

        [[nodiscard]] bool IsValid() const { return Error.empty(); }
    };

    [[nodiscard]] std::filesystem::path PathFromUtf8(std::string_view text);
    [[nodiscard]] std::string PathToUtf8(const std::filesystem::path& path);

    [[nodiscard]] std::filesystem::path GetAssetsDirectory(const std::filesystem::path& projectDirectory);

    [[nodiscard]] ProjectResult CreateProject(const std::filesystem::path& parentDirectory, std::string_view name);
    [[nodiscard]] ProjectResult OpenProject(const std::filesystem::path& projectDirectory);
}
