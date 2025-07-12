#include "Launcher.h"

#include <imgui/imgui.h>
#include <imgui/imgui_internal.h>

namespace QuasarEngine
{	
	Launcher::Launcher()
	{
		config = YAML::LoadFile("config.ultconf");
		m_ProjectManager = std::make_unique<ProjectManager>();
	}

	void Launcher::OnAttach()
	{
		
	}

	void Launcher::OnDetach()
	{

	}

	void Launcher::OnUpdate(double dt)
	{

	}

	void Launcher::OnGuiRender()
	{
		static bool dockspaceOpen = true;
		static bool opt_fullscreen = true;
		static bool opt_padding = false;
		static ImGuiDockNodeFlags dockspace_flags = ImGuiDockNodeFlags_AutoHideTabBar;

		ImGuiWindowFlags window_flags = ImGuiWindowFlags_MenuBar | ImGuiWindowFlags_NoDocking;
		if (opt_fullscreen)
		{
			ImGuiViewport* viewport = ImGui::GetMainViewport();
			ImGui::SetNextWindowPos(viewport->Pos);
			ImGui::SetNextWindowSize(viewport->Size);
			ImGui::SetNextWindowViewport(viewport->ID);
			ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 0.0f);
			ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 0.0f);
			window_flags |= ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove;
			window_flags |= ImGuiWindowFlags_NoBringToFrontOnFocus | ImGuiWindowFlags_NoNavFocus;

		}
		else
		{
			dockspace_flags &= ~ImGuiDockNodeFlags_PassthruCentralNode;
		}

		if (dockspace_flags & ImGuiDockNodeFlags_PassthruCentralNode)
			window_flags |= ImGuiWindowFlags_NoBackground;

		if (!opt_padding)
			ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0.0f, 0.0f));
		ImGui::Begin("DockSpace", &dockspaceOpen, window_flags);

		if (!opt_padding)
			ImGui::PopStyleVar();

		if (opt_fullscreen)
			ImGui::PopStyleVar(2);

		ImGuiIO& io = ImGui::GetIO();
		if (io.ConfigFlags & ImGuiConfigFlags_DockingEnable)
		{
			ImGuiID dockspace_id = ImGui::GetID("DockSpace");
			ImGui::DockSpace(dockspace_id, ImVec2(0.0f, 0.0f), dockspace_flags);

			static auto first_time = true;
			if (first_time) {
				ImGui::DockBuilderRemoveNode(dockspace_id); // Clear out existing layout
				ImGui::DockBuilderAddNode(dockspace_id, ImGuiDockNodeFlags_DockSpace); // Add empty node
				ImGui::DockBuilderSetNodeSize(dockspace_id, ImGui::GetMainViewport()->Size);

				auto dock_main_id = dockspace_id; // This variable will track the document node, however we are not using it here as we aren't docking windows into it
				auto dock_id_left = ImGui::DockBuilderSplitNode(dock_main_id, ImGuiDir_Left, 0.1f, nullptr, &dock_main_id);
				auto dock_id_right = ImGui::DockBuilderSplitNode(dock_main_id, ImGuiDir_Right, 0.7f, nullptr, &dock_id_left);

				ImGui::DockBuilderDockWindow("Create Project", dock_id_left);
				ImGui::DockBuilderDockWindow("Project Viewer", dock_id_right);

				ImGui::DockBuilderFinish(dockspace_id);
				first_time = false;
			}
		}

		ImGui::Begin("Create Project", false, ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoNavFocus);

		m_ProjectManager->CreateNewProject();

		m_ProjectManager->OnImGuiRender();

		ImGui::End();

		ImGui::Begin("Project Viewer", false, ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoNavFocus);
		if (config["recentProjects"])
		{
			for (auto project : config["recentProjects"])
			{
				ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.0f, 0.0f, 0.0f, 0.0f));
				ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.2f, 0.2f, 0.2f, 1));
				ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(0.1f, 0.1f, 0.1f, 1));

				ImGui::PushStyleVar(ImGuiStyleVar_ButtonTextAlign, ImVec2(0.0f, 0.5f));

				std::string label = project["Project"].as<std::string>() + std::string("\n") + project["Path"].as<std::string>();
				if (ImGui::Button(label.c_str(), ImVec2(ImGui::GetWindowSize().x, 70)))
				{
					m_ProjectManager->OpenProjectFromPath(project["Path"].as<std::string>());
				}

				ImGui::PopStyleColor(3);
				ImGui::PopStyleVar();
			}

		}

		ImGui::End();

		ImGui::End();
	}

	void Launcher::OnEvent(Event& e)
	{

	}
}