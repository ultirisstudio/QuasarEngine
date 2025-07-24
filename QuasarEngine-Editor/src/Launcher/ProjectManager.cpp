#include "ProjectManager.h"

#include "Tools/Utils.h"
#include <QuasarEngine/Core/Application.h>

#include <imgui/imgui.h>
#include <imgui/misc/cpp/imgui_stdlib.h>

#include <yaml-cpp/yaml.h>

#include <fstream>
#include <filesystem>
#include <iostream>
#include <cstdlib>

#include <Windows.h>

namespace QuasarEngine
{
	ProjectManager::ProjectManager()
		: m_Properties(nullptr), m_CreateNewProjectDialog(false), m_OpenProjectDialog(false)
	{
		tempProjectPath = GetDefaultProjectPath();
	}

	ProjectManager::~ProjectManager()
	{
	}

	void ProjectManager::CreateNewProject()
	{
		m_OpenProjectDialog = false;
		m_CreateNewProjectDialog = true;
	}

	void ProjectManager::OpenProject()
	{
		m_CreateNewProjectDialog = false;
		m_OpenProjectDialog = true;
	}

	void ProjectManager::OpenProjectFromPath(const std::string& projectPath)
	{
		std::string projectName = std::filesystem::path(projectPath + "\\").parent_path().filename().string();
		std::string projectFilePath = projectPath + "\\" + projectName + ".ultprj";

		if (std::filesystem::exists(projectFilePath))
		{
			std::ifstream stream(projectFilePath);
			std::stringstream strStream;
			strStream << stream.rdbuf();

			YAML::Node data = YAML::Load(strStream.str());

			m_Properties = std::make_unique<ProjectProperties>();
			m_Properties->m_ProjectName = data["Project"].as<std::string>();
			m_Properties->m_ProjectPath = data["Path"].as<std::string>();

			std::string cmdLine = "\"QuasarEngine-Editor.exe\" --project=\"" + m_Properties->m_ProjectPath + "\"";

			STARTUPINFOA si = { sizeof(si) };
			PROCESS_INFORMATION pi;

			BOOL success = CreateProcessA(
				NULL,
				cmdLine.data(),
				NULL, NULL,
				FALSE,
				DETACHED_PROCESS | CREATE_NEW_PROCESS_GROUP,
				NULL,
				NULL,
				&si,
				&pi
			);

			if (success)
			{
				CloseHandle(pi.hProcess);
				CloseHandle(pi.hThread);
				Application::Get().Close();
			}
			else
			{
				MessageBoxA(NULL, "Échec du lancement de l'éditeur", "Erreur", MB_ICONERROR);
			}
		}
	}

	void ProjectManager::OnImGuiRender()
	{
		if (!m_CreateNewProjectDialog && !m_OpenProjectDialog)
		{
			ImGui::Text("QuasarEngine Project Launcher");
			ImGui::Spacing();

			if (ImGui::Button("Create new project", ImVec2(250, 40)))
				CreateNewProject();

			if (ImGui::Button("Open project", ImVec2(250, 40)))
				OpenProject();

			return;
		}

		if (m_CreateNewProjectDialog)
		{
			ImGui::SeparatorText("Create new project");
			ImGui::Text("Name of the project");
			ImGui::InputText("##project_name", &tempProjectName);

			ImGui::Text("Project path");
			ImGui::InputText("##project_path", &tempProjectPath);
			ImGui::SameLine();
			if (ImGui::Button("...", ImVec2(30, 20)))
				tempProjectPath = Utils::openFolder();

			ImGui::Text("Path : %s\\%s", tempProjectPath.c_str(), tempProjectName.c_str());

			if (ImGui::Button("Create"))
			{
				std::string fullPath = tempProjectPath + "\\" + tempProjectName;
				if (!std::filesystem::exists(fullPath))
				{
					CreateProjectFromPath(tempProjectName, fullPath);
					m_CreateNewProjectDialog = false;
				}
			}
			ImGui::SameLine();
			if (ImGui::Button("Return"))
				m_CreateNewProjectDialog = false;
		}

		if (m_OpenProjectDialog)
		{
			ImGui::SeparatorText("Open project");
			ImGui::InputText("##project_path", &tempProjectPath);
			ImGui::SameLine();
			if (ImGui::Button("..."))
				tempProjectPath = Utils::openFolder();

			if (ImGui::Button("Open"))
			{
				OpenProjectFromPath(tempProjectPath);
				m_OpenProjectDialog = false;
			}
			ImGui::SameLine();
			if (ImGui::Button("Return"))
				m_OpenProjectDialog = false;
		}
	}

	void ProjectManager::CreateProjectFromPath(const std::string& projectName, const std::string& projectPath)
	{
		std::filesystem::create_directory(projectPath);
		std::filesystem::create_directory(projectPath + "\\Assets");
		std::filesystem::create_directory(projectPath + "\\Scripts");

		YAML::Emitter out;
		out << YAML::BeginMap;
		out << YAML::Key << "Project" << YAML::Value << projectName;
		out << YAML::Key << "Path" << YAML::Value << projectPath;
		out << YAML::EndMap;

		std::ofstream fout(projectPath + "\\" + projectName + ".ultprj");
		fout << out.c_str();
		fout.close();

		YAML::Node config;
		if (std::filesystem::exists("config.ultconf"))
			config = YAML::LoadFile("config.ultconf");

		YAML::Node recent = config["recentProjects"];
		recent.push_back(YAML::Load("{Project: " + projectName + ", Path: " + projectPath + "}"));
		config["recentProjects"] = recent;
		config["projectName"] = projectName;
		config["projectPath"] = projectPath;

		std::ofstream foutConfig("config.ultconf");
		foutConfig << config;
		foutConfig.close();

		std::string cmdLine = "\"QuasarEngine-Editor.exe\" --project=\"" + projectPath + "\"";

		STARTUPINFOA si = { sizeof(si) };
		PROCESS_INFORMATION pi;
		BOOL success = CreateProcessA(
			NULL,
			cmdLine.data(),
			NULL, NULL,
			FALSE,
			DETACHED_PROCESS | CREATE_NEW_PROCESS_GROUP,
			NULL,
			NULL,
			&si,
			&pi
		);

		if (success)
		{
			CloseHandle(pi.hProcess);
			CloseHandle(pi.hThread);
			Application::Get().Close();
		}
		else
		{
			MessageBoxA(NULL, "Échec du lancement de l'éditeur", "Erreur", MB_ICONERROR);
		}
	}

	std::string ProjectManager::GetDefaultProjectPath() const
	{
		const char* userProfile = std::getenv("USERPROFILE");
		if (userProfile)
			return std::string(userProfile) + "\\Documents\\Ultiris Projects";
		return "C:\\UltirisProjects";
	}
}
