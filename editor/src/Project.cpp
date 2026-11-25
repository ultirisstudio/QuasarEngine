#include "Project.h"

#include "EditorText.h"

#include <quasar/Version.h>
#include <quasar/io/File.h>
#include <quasar/io/Path.h>
#include <quasar/io/UserFolders.h>

#include <yaml-cpp/yaml.h>

#include <algorithm>
#include <optional>
#include <string>
#include <system_error>
#include <utility>
#include <vector>

namespace Quasar::Editor
{
    namespace
    {
        constexpr std::string_view NameKey = "Name";
        constexpr std::string_view FormatVersionKey = "FormatVersion";
        constexpr std::string_view EngineVersionKey = "EngineVersion";

        ProjectResult Fail(std::string error)
        {
            return {.Directory = {}, .ProjectFile = {}, .Name = {}, .Error = std::move(error)};
        }

        std::string Quote(const std::filesystem::path& path)
        {
            return "\"" + PathToUtf8(path) + "\"";
        }

        std::string FolderError(std::string_view key, const std::filesystem::path& folder, const std::error_code& error)
        {
            return Tr(key, {{"path", Quote(folder)}, {"reason", error.message()}});
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

        std::vector<std::filesystem::path> FindProjectFiles(const std::filesystem::path& projectDirectory, std::error_code& error)
        {
            std::vector<std::filesystem::path> files;
            for (const std::filesystem::directory_entry& entry : std::filesystem::directory_iterator(projectDirectory, error))
            {
                if (entry.path().extension() != ProjectFileExtension)
                {
                    continue;
                }
                const std::filesystem::file_type type = GetFileType(entry.path(), error);
                if (error)
                {
                    return {};
                }
                if (type == std::filesystem::file_type::regular)
                {
                    files.push_back(entry.path());
                }
            }
            return files;
        }

        std::string WriteProjectFile(std::string_view name)
        {
            YAML::Emitter out;
            out << YAML::BeginMap;
            out << YAML::Key << std::string(NameKey) << YAML::Value << std::string(name);
            out << YAML::Key << std::string(FormatVersionKey) << YAML::Value << ProjectFormatVersion;
            out << YAML::Key << std::string(EngineVersionKey) << YAML::Value << std::string(GetVersionString());
            out << YAML::EndMap;
            return std::string(out.c_str()) + "\n";
        }

        ProjectResult ReadProjectFile(const std::filesystem::path& file)
        {
            std::string error;
            const std::optional<std::string> text = ReadTextFile(file, error);
            if (!text)
            {
                return Fail(Tr("project.error.read_file", {{"reason", error}}));
            }

            YAML::Node root;
            try
            {
                root = YAML::Load(*text);
            }
            catch (const YAML::Exception& exception)
            {
                return Fail(Tr("project.error.invalid_yaml", {{"path", Quote(file)}, {"reason", exception.msg}}));
            }

            if (!root.IsMap())
            {
                return Fail(Tr("project.error.not_map", {{"path", Quote(file)}, {"name", std::string(NameKey)}, {"version", std::string(FormatVersionKey)}}));
            }

            const YAML::Node version = root[std::string(FormatVersionKey)];
            int formatVersion = 0;
            if (!version.IsScalar() || !YAML::convert<int>::decode(version, formatVersion) || formatVersion < 1)
            {
                return Fail(Tr("project.error.invalid_version", {{"path", Quote(file)}, {"version", std::string(FormatVersionKey)}}));
            }
            if (formatVersion > ProjectFormatVersion)
            {
                return Fail(Tr("project.error.newer_version",
                               {{"path", Quote(file)}, {"format", std::to_string(formatVersion)}, {"supported", std::to_string(ProjectFormatVersion)}}));
            }

            const YAML::Node name = root[std::string(NameKey)];
            if (!name.IsScalar() || name.Scalar().empty())
            {
                return Fail(Tr("project.error.missing_name", {{"path", Quote(file)}, {"name", std::string(NameKey)}}));
            }

            return {.Directory = {}, .ProjectFile = file, .Name = name.Scalar(), .Error = {}};
        }

        std::filesystem::path FindTopmostMissingDirectory(const std::filesystem::path& path, std::error_code& error)
        {
            std::filesystem::path missing;
            for (std::filesystem::path current = path; !current.empty(); current = current.parent_path())
            {
                const bool exists = std::filesystem::exists(current, error);
                if (exists || error)
                {
                    break;
                }
                missing = current;
                if (current.parent_path() == current)
                {
                    break;
                }
            }
            return missing;
        }

        // Folders that something else filled in the meantime are kept
        void RemoveCreatedDirectories(const std::filesystem::path& path, const std::filesystem::path& topmostCreated)
        {
            if (topmostCreated.empty())
            {
                return;
            }
            std::error_code ignored;
            for (std::filesystem::path current = path; current != topmostCreated && current.has_parent_path(); current = current.parent_path())
            {
                std::filesystem::remove(current, ignored);
            }
            std::filesystem::remove(topmostCreated, ignored);
        }

        ProjectResult FailAndRemove(const std::filesystem::path& directory, const std::filesystem::path& topmostCreated, std::string error)
        {
            std::error_code ignored;
            std::filesystem::remove_all(directory, ignored);
            RemoveCreatedDirectories(directory.parent_path(), topmostCreated);
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
        if (!location.is_absolute() || !IsValidFileName(name))
        {
            return {};
        }
        return (location / PathFromUtf8(name)).lexically_normal();
    }

    ProjectResult CreateProject(const std::filesystem::path& location, std::string_view name)
    {
        if (!IsValidFileName(name))
        {
            return Fail(Tr("project.error.invalid_name", {{"name", "\"" + std::string(name) + "\""}}));
        }
        if (location.empty())
        {
            return Fail(Tr("project.error.no_location"));
        }
        if (!location.is_absolute())
        {
            return Fail(Tr("project.error.relative_location", {{"path", Quote(location)}}));
        }

        const std::filesystem::path directory = ResolveProjectDirectory(location, name);
        const std::filesystem::path parent = directory.parent_path();
        std::error_code error;
        const std::filesystem::path topmostCreated = FindTopmostMissingDirectory(parent, error);
        if (error)
        {
            return Fail(FolderError("project.error.check_folder", parent, error));
        }
        std::filesystem::create_directories(parent, error);
        if (error)
        {
            RemoveCreatedDirectories(parent, topmostCreated);
            return Fail(FolderError("project.error.create_folder", parent, error));
        }

        // create_directory() checks and creates in one step, so a folder made at the same time by another program is
        // never taken as the new project
        const bool created = std::filesystem::create_directory(directory, error);
        if (error || !created)
        {
            RemoveCreatedDirectories(parent, topmostCreated);
            return Fail(error ? FolderError("project.error.create_folder", directory, error) : Tr("project.error.folder_exists", {{"path", Quote(directory)}}));
        }

        const std::filesystem::path assets = GetAssetsDirectory(directory);
        std::filesystem::create_directory(assets, error);
        if (error)
        {
            return FailAndRemove(directory, topmostCreated, FolderError("project.error.create_folder", assets, error));
        }

        const std::filesystem::path projectFile = GetProjectFile(directory);
        if (std::string reason; !WriteTextFile(projectFile, WriteProjectFile(name), reason))
        {
            return FailAndRemove(directory, topmostCreated, Tr("project.error.write_file", {{"reason", reason}}));
        }

        return {.Directory = directory, .ProjectFile = projectFile, .Name = std::string(name), .Error = {}};
    }

    ProjectResult OpenProject(const std::filesystem::path& projectDirectory)
    {
        if (projectDirectory.empty())
        {
            return Fail(Tr("project.error.no_folder"));
        }

        std::error_code error;
        std::filesystem::path directory = std::filesystem::absolute(projectDirectory, error).lexically_normal();
        if (error)
        {
            return Fail(FolderError("project.error.find_folder", projectDirectory, error));
        }
        if (!directory.has_filename())
        {
            directory = directory.parent_path();
        }

        const std::filesystem::file_type type = GetFileType(directory, error);
        if (error)
        {
            return Fail(FolderError("project.error.read_folder", directory, error));
        }
        if (type == std::filesystem::file_type::not_found)
        {
            return Fail(Tr("project.error.missing_folder", {{"path", Quote(directory)}}));
        }
        if (type != std::filesystem::file_type::directory)
        {
            return Fail(Tr("project.error.not_folder", {{"path", Quote(directory)}}));
        }

        const std::vector<std::filesystem::path> projectFiles = FindProjectFiles(directory, error);
        if (error)
        {
            return Fail(FolderError("project.error.read_folder", directory, error));
        }
        if (projectFiles.empty())
        {
            return Fail(Tr("project.error.not_project", {{"path", Quote(directory)}, {"extension", std::string(ProjectFileExtension)}}));
        }
        if (projectFiles.size() > 1)
        {
            return Fail(Tr("project.error.several_files", {{"path", Quote(directory)}, {"extension", std::string(ProjectFileExtension)}}));
        }

        ProjectResult project = ReadProjectFile(projectFiles.front());
        if (!project.IsValid())
        {
            return project;
        }

        const bool hasAssets = GetFileType(GetAssetsDirectory(directory), error) == std::filesystem::file_type::directory;
        if (error)
        {
            return Fail(FolderError("project.error.read_folder", GetAssetsDirectory(directory), error));
        }
        if (!hasAssets)
        {
            return Fail(Tr("project.error.no_assets", {{"path", Quote(directory)}, {"assets", std::string(AssetsDirectoryName)}}));
        }

        project.Directory = std::move(directory);
        return project;
    }
}
