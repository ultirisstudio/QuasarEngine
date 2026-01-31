#include "RecentProjectsStore.h"

#include <yaml-cpp/yaml.h>
#include <fstream>
#include <filesystem>
#include <algorithm>
#include <chrono>

namespace QuasarEngine
{
	static std::string ToLowerCopy(std::string s)
	{
		std::transform(s.begin(), s.end(), s.begin(), [](unsigned char c) { return (char)std::tolower(c); });
		return s;
	}

	RecentProjectsStore::RecentProjectsStore(std::string configPath) : m_ConfigPath(std::move(configPath))
	{
	}

	bool RecentProjectsStore::EnsureConfigFileExists(const std::string& configPath)
	{
		if (std::filesystem::exists(configPath))
			return true;

		YAML::Node root;
		root["projectName"] = "";
		root["projectPath"] = "";
		root["recentProjects"] = YAML::Node(YAML::NodeType::Sequence);
		root["settings"]["autoOpenLastProject"] = false;
		root["settings"]["showMissingProjects"] = true;
		root["settings"]["compactCards"] = false;

		std::ofstream out(configPath);
		if (!out.is_open())
			return false;

		out << root;
		return true;
	}

	std::uint64_t RecentProjectsStore::NowUnix()
	{
		using namespace std::chrono;
		return (std::uint64_t)duration_cast<seconds>(system_clock::now().time_since_epoch()).count();
	}

	std::string RecentProjectsStore::NormalizePath(std::string p)
	{
		std::replace(p.begin(), p.end(), '/', '\\');

		while (!p.empty() && (p.back() == '\\' || p.back() == ' '))
			p.pop_back();

		return ToLowerCopy(p);
	}

	std::string RecentProjectsStore::GuessProjectNameFromPath(const std::string& projectPath)
	{
		try
		{
			return std::filesystem::path(projectPath).filename().string();
		}
		catch (...)
		{
			return "Untitled";
		}
	}

	std::string RecentProjectsStore::ProjectFilePath(const std::string& projectPath, const std::string& projectName)
	{
		return projectPath + "\\" + projectName + ".ultprj";
	}

	bool RecentProjectsStore::ValidateProjectFolder(const std::string& projectPath, std::string* outProjectName)
	{
		if (!std::filesystem::exists(projectPath))
			return false;

		std::string guessed = GuessProjectNameFromPath(projectPath);
		std::string prjFile = ProjectFilePath(projectPath, guessed);

		if (!std::filesystem::exists(prjFile))
			return false;

		try
		{
			std::ifstream stream(prjFile);
			if (!stream.is_open())
				return false;

			std::stringstream ss;
			ss << stream.rdbuf();

			YAML::Node data = YAML::Load(ss.str());
			if (data["Project"])
			{
				if (outProjectName)
					*outProjectName = data["Project"].as<std::string>();
				return true;
			}
		}
		catch (...)
		{
			
		}

		if (outProjectName)
			*outProjectName = guessed;

		return true;
	}

	static RecentProject ReadRecent(const YAML::Node& n)
	{
		RecentProject rp;

		if (n["Project"]) rp.Name = n["Project"].as<std::string>();
		if (n["Path"])    rp.Path = n["Path"].as<std::string>();

		if (n["Name"]) rp.Name = n["Name"].as<std::string>();
		if (n["Pinned"]) rp.Pinned = n["Pinned"].as<bool>();
		if (n["LastOpenedUnix"]) rp.LastOpenedUnix = n["LastOpenedUnix"].as<std::uint64_t>();
		if (n["Thumbnail"]) rp.ThumbnailPath = n["Thumbnail"].as<std::string>();
		if (n["EngineVersion"]) rp.EngineVersion = n["EngineVersion"].as<std::string>();

		if (rp.Name.empty() && !rp.Path.empty())
			rp.Name = RecentProjectsStore::GuessProjectNameFromPath(rp.Path);

		return rp;
	}

	bool RecentProjectsStore::Load(LauncherConfig& outConfig)
	{
		if (!EnsureConfigFileExists(m_ConfigPath))
			return false;

		YAML::Node root;
		try
		{
			root = YAML::LoadFile(m_ConfigPath);
		}
		catch (...)
		{
			std::filesystem::remove(m_ConfigPath);
			if (!EnsureConfigFileExists(m_ConfigPath))
				return false;
			root = YAML::LoadFile(m_ConfigPath);
		}

		outConfig = LauncherConfig{};

		if (root["projectName"]) outConfig.ProjectName = root["projectName"].as<std::string>();
		if (root["projectPath"]) outConfig.ProjectPath = root["projectPath"].as<std::string>();

		if (root["settings"])
		{
			auto s = root["settings"];
			if (s["autoOpenLastProject"]) outConfig.Settings.AutoOpenLastProject = s["autoOpenLastProject"].as<bool>();
			if (s["showMissingProjects"]) outConfig.Settings.ShowMissingProjects = s["showMissingProjects"].as<bool>();
			if (s["compactCards"]) outConfig.Settings.CompactCards = s["compactCards"].as<bool>();
		}

		if (root["recentProjects"] && root["recentProjects"].IsSequence())
		{
			for (auto item : root["recentProjects"])
			{
				RecentProject rp = ReadRecent(item);
				if (!rp.Path.empty())
					outConfig.RecentProjects.push_back(std::move(rp));
			}
		}

		{
			std::vector<RecentProject> dedup;
			dedup.reserve(outConfig.RecentProjects.size());

			for (auto& rp : outConfig.RecentProjects)
			{
				std::string key = NormalizePath(rp.Path);
				bool exists = false;
				for (auto& d : dedup)
				{
					if (NormalizePath(d.Path) == key)
					{
						d.Pinned = d.Pinned || rp.Pinned;
						d.LastOpenedUnix = std::max(d.LastOpenedUnix, rp.LastOpenedUnix);
						if (d.Name.empty()) d.Name = rp.Name;
						if (d.ThumbnailPath.empty()) d.ThumbnailPath = rp.ThumbnailPath;
						if (d.EngineVersion.empty()) d.EngineVersion = rp.EngineVersion;
						exists = true;
						break;
					}
				}
				if (!exists)
					dedup.push_back(std::move(rp));
			}

			outConfig.RecentProjects = std::move(dedup);
		}

		return true;
	}

	bool RecentProjectsStore::Save(const LauncherConfig& config)
	{
		YAML::Node root;
		root["projectName"] = config.ProjectName;
		root["projectPath"] = config.ProjectPath;

		root["settings"]["autoOpenLastProject"] = config.Settings.AutoOpenLastProject;
		root["settings"]["showMissingProjects"] = config.Settings.ShowMissingProjects;
		root["settings"]["compactCards"] = config.Settings.CompactCards;

		YAML::Node recents(YAML::NodeType::Sequence);
		for (const auto& rp : config.RecentProjects)
		{
			YAML::Node n;
			n["Name"] = rp.Name;
			n["Path"] = rp.Path;
			n["Pinned"] = rp.Pinned;
			n["LastOpenedUnix"] = rp.LastOpenedUnix;
			if (!rp.ThumbnailPath.empty()) n["Thumbnail"] = rp.ThumbnailPath;
			if (!rp.EngineVersion.empty()) n["EngineVersion"] = rp.EngineVersion;

			n["Project"] = rp.Name;

			recents.push_back(n);
		}
		root["recentProjects"] = recents;

		std::ofstream out(m_ConfigPath);
		if (!out.is_open())
			return false;

		out << root;
		return true;
	}

	void RecentProjectsStore::Touch(LauncherConfig& config, const std::string& name, const std::string& path)
	{
		const std::string key = NormalizePath(path);

		for (auto it = config.RecentProjects.begin(); it != config.RecentProjects.end(); ++it)
		{
			if (NormalizePath(it->Path) == key)
			{
				it->Name = !name.empty() ? name : it->Name;
				it->Path = path;
				it->LastOpenedUnix = NowUnix();

				RecentProject updated = *it;
				config.RecentProjects.erase(it);
				config.RecentProjects.insert(config.RecentProjects.begin(), std::move(updated));
				return;
			}
		}

		RecentProject rp;
		rp.Name = !name.empty() ? name : GuessProjectNameFromPath(path);
		rp.Path = path;
		rp.LastOpenedUnix = NowUnix();
		config.RecentProjects.insert(config.RecentProjects.begin(), std::move(rp));
	}

	void RecentProjectsStore::Remove(LauncherConfig& config, const std::string& path)
	{
		const std::string key = NormalizePath(path);
		config.RecentProjects.erase(
			std::remove_if(config.RecentProjects.begin(), config.RecentProjects.end(),
				[&](const RecentProject& rp) { return NormalizePath(rp.Path) == key; }),
			config.RecentProjects.end()
		);
	}

	void RecentProjectsStore::SetPinned(LauncherConfig& config, const std::string& path, bool pinned)
	{
		const std::string key = NormalizePath(path);
		for (auto& rp : config.RecentProjects)
		{
			if (NormalizePath(rp.Path) == key)
			{
				rp.Pinned = pinned;
				return;
			}
		}
	}
}
