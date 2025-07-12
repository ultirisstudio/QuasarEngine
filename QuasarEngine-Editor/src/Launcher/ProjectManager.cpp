#include "ProjectManager.h"

#include "Tools/Utils.h"

#include <iostream>
#include <fstream>
#include <filesystem>

#include <yaml-cpp/yaml.h>

#include <imgui/imgui.h>
#include <imgui/misc/cpp/imgui_stdlib.h>

#include <QuasarEngine/Core/Application.h>
#include <QuasarEngine/Renderer/Renderer.h>

#include <Windows.h>

namespace QuasarEngine
{
	ProjectManager::ProjectManager() : m_Properties(nullptr), m_CreateNewProjectDialog(false), m_OpenProjectDialog(false)
	{

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

	void ProjectManager::OnImGuiRender()
	{
		if (m_CreateNewProjectDialog)
		{
			ImGui::Text("Project name");
			ImGui::InputText("##project_name", &tempProjectName);
			ImGui::Text("Project path");
			ImGui::InputText("##project_path", &tempProjectPath);
			ImGui::SameLine();
			if (ImGui::Button("...", ImVec2(30, 20)))
			{
				tempProjectPath = Utils::openFolder();
			}

			ImVec4 color = ImVec4(0.5f, 0.5f, 0.5f, 1.0f);
			ImGui::PushStyleColor(ImGuiCol_Text, color);
			ImGui::Text("Project location: %s\\%s", tempProjectPath.c_str(), tempProjectName.c_str());
			ImGui::PopStyleColor();

			if (ImGui::Button("Create Project"))
			{
				std::string fullPath = tempProjectPath + std::string("\\") + tempProjectName;

				if (!std::filesystem::exists(fullPath))
				{
					m_Properties = new ProjectProperties();
					m_Properties->m_ProjectName = tempProjectName;
					m_Properties->m_ProjectPath = fullPath;
					
					std::filesystem::create_directory(m_Properties->m_ProjectPath);
					std::filesystem::create_directory(m_Properties->m_ProjectPath + std::string("\\Assets"));
					std::filesystem::create_directory(m_Properties->m_ProjectPath + std::string("\\Scripts"));

					CreateProjectFiles(m_Properties->m_ProjectName, m_Properties->m_ProjectPath);

					std::filesystem::path path(m_Properties->m_ProjectPath + "\\Scripts\\Build\\" + m_Properties->m_ProjectName + ".dll");
					
					YAML::Emitter out;
					out << YAML::BeginMap;
					out << YAML::Key << "Project" << YAML::Value << m_Properties->m_ProjectName;
					out << YAML::Key << "Path" << YAML::Value << m_Properties->m_ProjectPath;
					out << YAML::EndMap;

					std::ofstream fout(m_Properties->m_ProjectPath + std::string("\\" + m_Properties->m_ProjectName + std::string(".ultprj")));
					fout << out.c_str();
					
					YAML::Node config = YAML::LoadFile("config.ultconf");
					if (config["recentProjects"])
					{
						YAML::Node recentProjects = config["recentProjects"];
						recentProjects.push_back(YAML::Load("{Project: " + m_Properties->m_ProjectName + ", Path: " + m_Properties->m_ProjectPath + "}"));
						config["recentProjects"] = recentProjects;
					}
					else
					{
						YAML::Node recentProjects;
						recentProjects.push_back(YAML::Load("{Project: " + m_Properties->m_ProjectName + ", Path: " + m_Properties->m_ProjectPath + "}"));
						config["recentProjects"] = recentProjects;
					}
					config["projectName"] = m_Properties->m_ProjectName;
					config["projectPath"] = m_Properties->m_ProjectPath;
					std::ofstream foutConfig("config.ultconf");
					foutConfig << config;
					foutConfig.close();

				}

				m_CreateNewProjectDialog = false;
			}
		}

		if (m_OpenProjectDialog)
		{
			ImGui::InputText("##project_path", &tempProjectPath);
			ImGui::SameLine();
			if (ImGui::Button("..."))
			{
				tempProjectPath = Utils::openFolder();
			}
			if (ImGui::Button("Open Project"))
			{
				OpenProjectFromPath(tempProjectPath);

				m_OpenProjectDialog = false;
			}
		}
	}

	void ProjectManager::CreateProjectFiles(const std::string& projectName, const std::string& projectPath)
	{
		std::string luaFileName = "premake5.lua";
		std::ofstream luaFile(projectPath + "\\Scripts\\" + luaFileName);
		std::string enginePath = std::filesystem::current_path().generic_string();

		luaFile << "include (\"" << enginePath << "/premake_customization/solution_items.lua\")\n";
		luaFile << "\n";
		luaFile << "Library = {}\n";
		luaFile << "Library[\"ScriptCore\"] = \"" << enginePath << "/Scripts/QuasarEngine-ScriptCore.dll\"\n";
		luaFile << "\n";
		luaFile << "workspace \"" << projectName << "\"\n";
		luaFile << "    architecture \"x86_64\"\n";
		luaFile << "    startproject \"" << projectName << "\"\n";
		luaFile << "\n";
		luaFile << "    configurations\n";
		luaFile << "    {\n";
		luaFile << "        \"Debug\",\n";
		luaFile << "        \"Release\"\n";
		luaFile << "    }\n";
		luaFile << "    flags\n";
		luaFile << "    {\n";
		luaFile << "        \"MultiProcessorCompile\"\n";
		luaFile << "    }\n";
		luaFile << "\n";
		luaFile << "project \"" << projectName << "\"\n";
		luaFile << "	kind \"SharedLib\"\n";
		luaFile << "	language \"C#\"\n";
		luaFile << "	dotnetframework \"4.8\"\n";
		luaFile << "	csversion (\"10.0\")\n";
		luaFile << "\n";
		luaFile << "	targetdir(\"%{wks.location}/Build\")\n";
		luaFile << "	objdir(\"%{wks.location}/Build/Intermediates\")\n";
		luaFile << "\n";
		luaFile << "	files\n";
		luaFile << "	{\n";
		luaFile << "	    \"Source/**.cs\",\n";
		luaFile << "	    \"Properties/**.cs\"\n";
		luaFile << "	}\n";
		luaFile << "\n";
		luaFile << "	links\n";
		luaFile << "	{\n";
		luaFile << "		\"%{Library.ScriptCore}\"\n";
		luaFile << "	}\n";
		luaFile << "	filter \"configurations:Debug\"\n";
		luaFile << "	    optimize \"Off\"\n";
		luaFile << "	    symbols \"Default\"\n";
		luaFile << "\n";
		luaFile << "	filter \"configurations:Release\"\n";
		luaFile << "	    optimize \"On\"\n";
		luaFile << "	    symbols \"Default\"\n";
		luaFile.close();

		std::string batFileName = "GenerateSolution.bat";
		std::ofstream batFile(projectPath + "\\" + batFileName);

		batFile << "@echo off\n";
		batFile << "pushd Scripts\n";
		batFile << "call " << enginePath << "\\premake\\premake5.exe vs2022\n";
		batFile << "popd\n";
		batFile << "PAUSE\n";
		batFile.close();
	}

	void ProjectManager::OpenProjectFromPath(const std::string& projectPath)
	{
		std::string projectName = std::filesystem::path(projectPath + "\\").parent_path().filename().string();
		std::string projectFilePath = projectPath + "\\" + projectName + ".ultprj";

		bool projectExists = std::filesystem::exists(projectFilePath);

		if (projectExists)
		{
			std::ifstream stream(projectFilePath);
			std::stringstream strStream;
			strStream << stream.rdbuf();

			YAML::Node data = YAML::Load(strStream.str());

			m_Properties = new ProjectProperties();
			m_Properties->m_ProjectName = data["Project"].as<std::string>();
			m_Properties->m_ProjectPath = data["Path"].as<std::string>();

			std::string command;
			command += std::filesystem::current_path().generic_string() + "\\OpenGLEditor.exe";
			//command += " --projectName=" + m_Properties->m_ProjectName;
			//command += " --projectPath=" + m_Properties->m_ProjectPath;

			//Application::Get().Close();
		}
	}
}