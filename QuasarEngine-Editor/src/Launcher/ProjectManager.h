#pragma once

#include <string>
#include <vector>
#include <memory>
#include <optional>

namespace QuasarEngine
{
	struct ProjectProperties
	{
		std::string m_ProjectPath;
		std::string m_ProjectName;
	};

	class ProjectManager
	{
	public:
		ProjectManager();
		~ProjectManager();

		void CreateNewProject();
		void OpenProject();
		void OpenProjectFromPath(const std::string& projectPath);
		void CreateProjectFromPath(const std::string& projectName, const std::string& projectPath);

		ProjectProperties* GetProjectProperties() { return m_Properties.get(); }

		void OnImGuiRender();

	private:
		std::string GetDefaultProjectPath() const;

	private:
		std::unique_ptr<ProjectProperties> m_Properties;

		std::string tempProjectName = "";
		std::string tempProjectPath;

		bool m_CreateNewProjectDialog = false;
		bool m_OpenProjectDialog = false;
	};
}
