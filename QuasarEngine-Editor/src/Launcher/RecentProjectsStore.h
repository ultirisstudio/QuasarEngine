#pragma once

#include <string>
#include <vector>
#include <cstdint>
#include <optional>

namespace QuasarEngine
{
	struct RecentProject
	{
		std::string Name;
		std::string Path;

		bool Pinned = false;
		std::uint64_t LastOpenedUnix = 0;
		std::string ThumbnailPath;
		std::string EngineVersion;
	};

	struct LauncherSettings
	{
		bool AutoOpenLastProject = false;
		bool ShowMissingProjects = true;
		bool CompactCards = false;
	};

	struct LauncherConfig
	{
		std::string ProjectName;
		std::string ProjectPath;

		std::vector<RecentProject> RecentProjects;

		LauncherSettings Settings;
	};

	class RecentProjectsStore
	{
	public:
		explicit RecentProjectsStore(std::string configPath = "config.ultconf");
		bool Load(LauncherConfig& outConfig);
		bool Save(const LauncherConfig& config);

		void Touch(LauncherConfig& config, const std::string& name, const std::string& path);

		void Remove(LauncherConfig& config, const std::string& path);

		void SetPinned(LauncherConfig& config, const std::string& path, bool pinned);

		static std::uint64_t NowUnix();
		static std::string NormalizePath(std::string p);
		static std::string GuessProjectNameFromPath(const std::string& projectPath);
		static std::string ProjectFilePath(const std::string& projectPath, const std::string& projectName);
		static bool ValidateProjectFolder(const std::string& projectPath, std::string* outProjectName = nullptr);

	private:
		std::string m_ConfigPath;

	private:
		static bool EnsureConfigFileExists(const std::string& configPath);
	};
}
