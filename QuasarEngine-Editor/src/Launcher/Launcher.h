#pragma once

#include <QuasarEngine.h>
#include <yaml-cpp/yaml.h>

#include "RecentProjectsStore.h"

#include <string>
#include <optional>

namespace QuasarEngine
{
	class Launcher : public Layer
	{
	public:
		Launcher();
		~Launcher() override = default;

		void OnAttach() override;
		void OnDetach() override;
		void OnUpdate(double dt) override;
		void OnGuiRender() override;
		void OnEvent(Event& e) override;

	private:
		enum class Page
		{
			Home = 0,
			Projects,
			Templates,
			Settings
		};

		struct TemplateDef
		{
			const char* Name = "";
			const char* Desc = "";
			const char* Tag = "";
			int Id = 0;
		};

	private:
		void ApplyQuasarHubTheme();
		void DrawDockspace();
		void DrawHeader();
		void DrawSidebar();
		void DrawHome();
		void DrawProjects();
		void DrawTemplates();
		void DrawSettings();

		void DrawRecentGrid(float cardW, float cardH);
		bool DrawProjectCard(int idx, const RecentProject& rp, float w, float h);
		void DrawDetailsPanel();

		void OpenCreateModal(int templateId = 0);
		void OpenOpenModal();
		void DrawCreateModal();
		void DrawOpenModal();

		bool CreateProjectOnDisk(const std::string& projectName, const std::string& projectPath, int templateId);
		void LaunchEditorWithProject(const std::string& projectName, const std::string& projectPath);
		void OpenInExplorer(const std::string& folderPath);

		static std::string HumanTimeAgo(std::uint64_t unixSeconds);
		static std::string BytesToNice(std::uint64_t bytes);
		static std::optional<std::uint64_t> TryComputeFolderSizeOnce(const std::string& folder);

	private:
		RecentProjectsStore m_Store;
		LauncherConfig m_Config;

		Page m_Page = Page::Home;

		std::string m_Search;
		bool m_ShowOnlyPinned = false;

		int m_SelectedIndex = -1;
		std::optional<std::uint64_t> m_SelectedFolderSize;

		bool m_CreateModalOpen = false;
		bool m_OpenModalOpen = false;

		std::string m_NewProjectName;
		std::string m_NewProjectBasePath;
		int m_NewProjectTemplate = 0;

		std::string m_OpenProjectPath;
		std::string m_LastError;

		TemplateDef m_Templates[4];
	};
}
