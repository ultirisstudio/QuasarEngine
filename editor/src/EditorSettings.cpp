#include "EditorSettings.h"

#include <quasar/core/Application.h>
#include <quasar/io/File.h>
#include <quasar/io/Path.h>
#include <quasar/log/Log.h>

#include <yaml-cpp/yaml.h>

#include <algorithm>
#include <iterator>
#include <optional>
#include <string>
#include <system_error>
#include <utility>

namespace Quasar::Editor
{
    namespace
    {
        constexpr const char* SettingsFileName = "EditorSettings.yaml";
        constexpr const char* LastLocationKey = "LastLocation";
        constexpr const char* RecentProjectsKey = "RecentProjects";

        std::filesystem::path ReadPath(const YAML::Node& node)
        {
            if (!node.IsScalar())
            {
                return {};
            }
            const std::filesystem::path path = PathFromUtf8(node.Scalar());
            return path.is_absolute() ? path.lexically_normal() : std::filesystem::path();
        }
    }

    EditorSettings::EditorSettings(std::filesystem::path file) : m_File(std::move(file))
    {
        Load();
    }

    void EditorSettings::RememberProject(const std::filesystem::path& projectDirectory)
    {
        const std::filesystem::path directory = projectDirectory.lexically_normal();
        m_LastLocation = directory.parent_path();
        std::erase(m_RecentProjects, directory);
        if (m_RecentProjects.size() >= MaxRecentProjects)
        {
            m_RecentProjects.resize(MaxRecentProjects - 1);
        }
        m_RecentProjects.push_back(directory);
        std::ranges::rotate(m_RecentProjects, std::prev(m_RecentProjects.end()));
        Save();
    }

    void EditorSettings::ForgetProject(const std::filesystem::path& projectDirectory)
    {
        if (std::erase(m_RecentProjects, projectDirectory.lexically_normal()) > 0)
        {
            Save();
        }
    }

    void EditorSettings::Load()
    {
        std::error_code error;
        if (m_File.empty() || !std::filesystem::exists(m_File, error))
        {
            return;
        }

        const std::optional<std::string> text = ReadTextFile(m_File);
        if (!text)
        {
            return;
        }

        YAML::Node root;
        try
        {
            root = YAML::Load(*text);
        }
        catch (const YAML::Exception& exception)
        {
            QUASAR_WARN("The editor settings '{}' are not valid YAML and are reset: {}", PathToUtf8(m_File), exception.msg);
            return;
        }
        if (!root.IsMap())
        {
            return;
        }

        m_LastLocation = ReadPath(root[LastLocationKey]);
        const YAML::Node recent = root[RecentProjectsKey];
        if (!recent.IsSequence())
        {
            return;
        }
        for (const YAML::Node& node : recent)
        {
            std::filesystem::path project = ReadPath(node);
            if (!project.empty() && std::ranges::find(m_RecentProjects, project) == m_RecentProjects.end() && m_RecentProjects.size() < MaxRecentProjects)
            {
                m_RecentProjects.push_back(std::move(project));
            }
        }
    }

    void EditorSettings::Save() const
    {
        if (m_File.empty())
        {
            return;
        }

        YAML::Emitter out;
        out << YAML::BeginMap;
        out << YAML::Key << LastLocationKey << YAML::Value << PathToUtf8(m_LastLocation);
        out << YAML::Key << RecentProjectsKey << YAML::Value << YAML::BeginSeq;
        for (const std::filesystem::path& project : m_RecentProjects)
        {
            out << PathToUtf8(project);
        }
        out << YAML::EndSeq << YAML::EndMap;

        if (!WriteTextFile(m_File, std::string(out.c_str()) + "\n"))
        {
            QUASAR_WARN("The editor settings are not saved");
        }
    }

    std::filesystem::path GetEditorSettingsFile()
    {
        const std::optional<std::filesystem::path> directory = Application::Get().GetUserDataDirectory();
        return directory ? *directory / SettingsFileName : std::filesystem::path();
    }
}
