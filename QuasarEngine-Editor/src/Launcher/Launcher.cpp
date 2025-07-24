#include "Launcher.h"
#include <imgui/imgui.h>
#include <imgui/imgui_internal.h>

namespace QuasarEngine {

	Launcher::Launcher() {
		if (std::filesystem::exists("config.ultconf"))
			m_Config = YAML::LoadFile("config.ultconf");
	}

	void Launcher::OnAttach() {
		
	}

	void Launcher::OnDetach() {
		
	}

	void Launcher::OnUpdate(double dt) {
		
	}

	void Launcher::OnGuiRender() {
		bool dockspaceOpen = true;
		bool opt_fullscreen = true;
		bool opt_padding = false;

		ImGuiWindowFlags window_flags = ImGuiWindowFlags_MenuBar | ImGuiWindowFlags_NoDocking;
		ImGuiDockNodeFlags dockspace_flags = ImGuiDockNodeFlags_AutoHideTabBar;

		if (opt_fullscreen) {
			ImGuiViewport* viewport = ImGui::GetMainViewport();
			ImGui::SetNextWindowPos(viewport->Pos);
			ImGui::SetNextWindowSize(viewport->Size);
			ImGui::SetNextWindowViewport(viewport->ID);
			ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 0.0f);
			ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 0.0f);

			window_flags |= ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoCollapse |
				ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove |
				ImGuiWindowFlags_NoBringToFrontOnFocus | ImGuiWindowFlags_NoNavFocus;
		}
		else {
			dockspace_flags &= ~ImGuiDockNodeFlags_PassthruCentralNode;
		}

		if (dockspace_flags & ImGuiDockNodeFlags_PassthruCentralNode)
			window_flags |= ImGuiWindowFlags_NoBackground;

		if (!opt_padding)
			ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0.0f, 0.0f));

		ImGui::Begin("LauncherDockspace", &dockspaceOpen, window_flags);

		if (!opt_padding)
			ImGui::PopStyleVar();
		if (opt_fullscreen)
			ImGui::PopStyleVar(2);

		ImGuiIO& io = ImGui::GetIO();
		if (io.ConfigFlags & ImGuiConfigFlags_DockingEnable) {
			ImGuiID dockspace_id = ImGui::GetID("LauncherDockspace");
			ImGui::DockSpace(dockspace_id, ImVec2(0.0f, 0.0f), dockspace_flags);

			static bool first_time = true;
			if (first_time) {
				ImGui::DockBuilderRemoveNode(dockspace_id);
				ImGui::DockBuilderAddNode(dockspace_id, ImGuiDockNodeFlags_DockSpace);
				ImGui::DockBuilderSetNodeSize(dockspace_id, ImGui::GetMainViewport()->Size);

				ImGuiID left, right;
				ImGuiID main_id = dockspace_id;
				left = ImGui::DockBuilderSplitNode(main_id, ImGuiDir_Left, 0.35f, nullptr, &main_id);
				right = ImGui::DockBuilderSplitNode(main_id, ImGuiDir_Right, 0.65f, nullptr, &main_id);

				ImGui::DockBuilderDockWindow("Create/Open Project", left);
				ImGui::DockBuilderDockWindow("Recent Projects", right);

				ImGui::DockBuilderFinish(dockspace_id);
				first_time = false;
			}
		}

		ImGui::Begin("Create/Open Project");
		m_ProjectManager.OnImGuiRender();
		ImGui::End();

		ImGui::Begin("Project Viewer");

		if (m_Config["recentProjects"])
		{
			for (auto project : m_Config["recentProjects"])
			{
				ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0, 0, 0, 0));
				ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.2f, 0.2f, 0.2f, 1.0f));
				ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(0.1f, 0.1f, 0.1f, 1.0f));
				ImGui::PushStyleVar(ImGuiStyleVar_ButtonTextAlign, ImVec2(0.0f, 0.5f));

				std::string label = project["Project"].as<std::string>() + "\n" + project["Path"].as<std::string>();
				if (ImGui::Button(label.c_str(), ImVec2(ImGui::GetContentRegionAvail().x, 70)))
				{
					m_ProjectManager.OpenProjectFromPath(project["Path"].as<std::string>());
				}

				ImGui::PopStyleColor(3);
				ImGui::PopStyleVar();
			}
		}
		else
		{
			ImGui::Text("No recent projects found.");
		}

		ImGui::End();
		ImGui::End();
	}

	void Launcher::OnEvent(Event& e) {
		
	}
}
