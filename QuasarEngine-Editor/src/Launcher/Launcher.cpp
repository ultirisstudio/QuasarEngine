#include "Launcher.h"

#include <QuasarEngine/Core/Application.h>
#include <QuasarEngine/Tools/Utils.h>

#include <imgui/imgui.h>
#include <imgui/misc/cpp/imgui_stdlib.h>

#include <filesystem>
#include <fstream>
#include <sstream>
#include <algorithm>
#include <chrono>

#ifdef _WIN32
#include <Windows.h>
#include <shellapi.h>
#endif

namespace QuasarEngine
{
	static std::string GetDefaultProjectPath()
	{
#ifdef _WIN32
		const char* userProfile = std::getenv("USERPROFILE");
		if (userProfile)
			return std::string(userProfile) + "\\Documents\\Ultiris Projects";
#endif
		return Utils::openFolder();
	}

	static std::string ToLowerCopy(std::string s)
	{
		std::transform(s.begin(), s.end(), s.begin(),
			[](unsigned char c) { return (char)std::tolower(c); });
		return s;
	}

	static std::string GetExePath()
	{
#ifdef _WIN32
		char buffer[MAX_PATH];
		DWORD len = GetModuleFileNameA(NULL, buffer, MAX_PATH);
		if (len > 0)
			return std::string(buffer, buffer + len);
#endif
		return "QuasarEngine-Editor.exe";
	}

	static bool IsMissing(const std::string& p)
	{
		return !std::filesystem::exists(p);
	}

	static void SortRecents(std::vector<RecentProject>& v)
	{
		std::stable_sort(v.begin(), v.end(), [](const RecentProject& a, const RecentProject& b)
			{
				if (a.Pinned != b.Pinned) return a.Pinned > b.Pinned;
				if (a.LastOpenedUnix != b.LastOpenedUnix) return a.LastOpenedUnix > b.LastOpenedUnix;
				return a.Name < b.Name;
			});
	}

	Launcher::Launcher() : m_Store("config.ultconf")
	{
		m_Templates[0] = { "Empty",      "Projet minimal (Assets/Scripts + .ultprj).", "Starter", 0 };
		m_Templates[1] = { "2D Starter", "Structure de base 2D: scenes + dossier textures.", "2D", 1 };
		m_Templates[2] = { "3D Starter", "Structure de base 3D: scenes + meshes + materials.", "3D", 2 };
		m_Templates[3] = { "UI Proto",   "UI sandbox: layout, widgets, test scenes.", "UI", 3 };
	}

	void Launcher::OnAttach()
	{
		m_Store.Load(m_Config);
		SortRecents(m_Config.RecentProjects);
		ApplyQuasarHubTheme();

		if (m_NewProjectBasePath.empty())
			m_NewProjectBasePath = GetDefaultProjectPath();

		if (m_Config.Settings.AutoOpenLastProject &&
			!m_Config.ProjectPath.empty() &&
			RecentProjectsStore::ValidateProjectFolder(m_Config.ProjectPath))
		{
			std::string pname;
			RecentProjectsStore::ValidateProjectFolder(m_Config.ProjectPath, &pname);
			LaunchEditorWithProject(pname, m_Config.ProjectPath);
		}
	}

	void Launcher::OnDetach() {}
	void Launcher::OnUpdate(double) {}
	void Launcher::OnEvent(Event&) {}

	void StyleColorsQuasarRustic(ImGuiStyle* dst)
	{
		
	}

	void Launcher::ApplyQuasarHubTheme()
	{
		ImGuiStyle* style = &ImGui::GetStyle();
		ImVec4* colors = style->Colors;

		style->WindowRounding = 4.0f;
		style->ChildRounding = 3.0f;
		style->FrameRounding = 3.0f;
		style->PopupRounding = 3.0f;
		style->ScrollbarRounding = 3.0f;
		style->GrabRounding = 3.0f;
		style->TabRounding = 3.0f;

		style->WindowBorderSize = 1.0f;
		style->FrameBorderSize = 1.0f;
		style->PopupBorderSize = 1.0f;

		style->WindowPadding = ImVec2(10.0f, 10.0f);
		style->FramePadding = ImVec2(8.0f, 6.0f);
		style->ItemSpacing = ImVec2(8.0f, 6.0f);
		style->ItemInnerSpacing = ImVec2(6.0f, 4.0f);
		style->IndentSpacing = 16.0f;

		const ImVec4 bg0 = ImVec4(0.085f, 0.090f, 0.095f, 1.00f);
		const ImVec4 bg1 = ImVec4(0.110f, 0.115f, 0.125f, 1.00f);
		const ImVec4 bg2 = ImVec4(0.145f, 0.150f, 0.165f, 1.00f);
		const ImVec4 bg3 = ImVec4(0.170f, 0.175f, 0.195f, 1.00f);
		const ImVec4 border = ImVec4(0.260f, 0.270f, 0.300f, 0.85f);

		const ImVec4 accent = ImVec4(0.24f, 0.52f, 0.88f, 1.00f);
		const ImVec4 accentSoft = ImVec4(0.24f, 0.52f, 0.88f, 0.55f);
		const ImVec4 accentSofter = ImVec4(0.24f, 0.52f, 0.88f, 0.30f);

		colors[ImGuiCol_Text] = ImVec4(0.92f, 0.93f, 0.95f, 1.00f);
		colors[ImGuiCol_TextDisabled] = ImVec4(0.55f, 0.56f, 0.58f, 1.00f);

		colors[ImGuiCol_WindowBg] = bg0;
		colors[ImGuiCol_ChildBg] = ImVec4(0.00f, 0.00f, 0.00f, 0.00f);
		colors[ImGuiCol_PopupBg] = ImVec4(0.08f, 0.085f, 0.095f, 0.98f);

		colors[ImGuiCol_Border] = border;
		colors[ImGuiCol_BorderShadow] = ImVec4(0, 0, 0, 0);

		colors[ImGuiCol_FrameBg] = bg1;
		colors[ImGuiCol_FrameBgHovered] = bg2;
		colors[ImGuiCol_FrameBgActive] = bg3;

		colors[ImGuiCol_TitleBg] = ImVec4(0.070f, 0.072f, 0.078f, 1.00f);
		colors[ImGuiCol_TitleBgActive] = ImVec4(0.080f, 0.082f, 0.090f, 1.00f);
		colors[ImGuiCol_TitleBgCollapsed] = ImVec4(0.070f, 0.072f, 0.078f, 1.00f);

		colors[ImGuiCol_MenuBarBg] = ImVec4(0.080f, 0.082f, 0.090f, 1.00f);

		colors[ImGuiCol_ScrollbarBg] = ImVec4(0.06f, 0.065f, 0.075f, 0.80f);
		colors[ImGuiCol_ScrollbarGrab] = ImVec4(0.24f, 0.25f, 0.28f, 1.00f);
		colors[ImGuiCol_ScrollbarGrabHovered] = ImVec4(0.30f, 0.31f, 0.35f, 1.00f);
		colors[ImGuiCol_ScrollbarGrabActive] = ImVec4(0.34f, 0.35f, 0.40f, 1.00f);

		colors[ImGuiCol_CheckMark] = accent;
		colors[ImGuiCol_SliderGrab] = ImVec4(accent.x, accent.y, accent.z, 0.85f);
		colors[ImGuiCol_SliderGrabActive] = accent;

		colors[ImGuiCol_Button] = ImVec4(0.145f, 0.150f, 0.165f, 1.00f);
		colors[ImGuiCol_ButtonHovered] = ImVec4(0.185f, 0.190f, 0.210f, 1.00f);
		colors[ImGuiCol_ButtonActive] = ImVec4(0.125f, 0.130f, 0.145f, 1.00f);

		colors[ImGuiCol_Header] = ImVec4(0.150f, 0.155f, 0.175f, 1.00f);
		colors[ImGuiCol_HeaderHovered] = ImVec4(0.195f, 0.200f, 0.225f, 1.00f);
		colors[ImGuiCol_HeaderActive] = ImVec4(0.135f, 0.140f, 0.160f, 1.00f);

		colors[ImGuiCol_Separator] = border;
		colors[ImGuiCol_SeparatorHovered] = accentSoft;
		colors[ImGuiCol_SeparatorActive] = accent;

		colors[ImGuiCol_ResizeGrip] = accentSofter;
		colors[ImGuiCol_ResizeGripHovered] = accentSoft;
		colors[ImGuiCol_ResizeGripActive] = accent;

		colors[ImGuiCol_Tab] = ImVec4(0.110f, 0.115f, 0.130f, 1.00f);
		colors[ImGuiCol_TabHovered] = ImVec4(0.170f, 0.175f, 0.195f, 1.00f);
		colors[ImGuiCol_TabActive] = ImVec4(0.135f, 0.140f, 0.160f, 1.00f);
		colors[ImGuiCol_TabUnfocused] = ImVec4(0.095f, 0.100f, 0.112f, 1.00f);
		colors[ImGuiCol_TabUnfocusedActive] = ImVec4(0.120f, 0.125f, 0.140f, 1.00f);

		colors[ImGuiCol_DockingPreview] = ImVec4(accent.x, accent.y, accent.z, 0.35f);
		colors[ImGuiCol_DockingEmptyBg] = ImVec4(0.10f, 0.105f, 0.115f, 1.00f);

		colors[ImGuiCol_PlotLines] = ImVec4(0.61f, 0.61f, 0.61f, 1.00f);
		colors[ImGuiCol_PlotLinesHovered] = ImVec4(1.00f, 0.43f, 0.35f, 1.00f);
		colors[ImGuiCol_PlotHistogram] = ImVec4(0.90f, 0.70f, 0.00f, 1.00f);
		colors[ImGuiCol_PlotHistogramHovered] = ImVec4(1.00f, 0.60f, 0.00f, 1.00f);

		colors[ImGuiCol_TextSelectedBg] = ImVec4(accent.x, accent.y, accent.z, 0.25f);
		colors[ImGuiCol_DragDropTarget] = ImVec4(1.00f, 1.00f, 0.00f, 0.90f);

		colors[ImGuiCol_NavHighlight] = accent;
		colors[ImGuiCol_NavWindowingHighlight] = ImVec4(1, 1, 1, 0.70f);
		colors[ImGuiCol_NavWindowingDimBg] = ImVec4(0.80f, 0.80f, 0.80f, 0.20f);
		colors[ImGuiCol_ModalWindowDimBg] = ImVec4(0.0f, 0.0f, 0.0f, 0.55f);
	}

	void Launcher::OnGuiRender()
	{
		DrawDockspace();
	}

	void Launcher::DrawDockspace()
	{
		bool open = true;
		ImGuiDockNodeFlags dockFlags = ImGuiDockNodeFlags_None;

		ImGuiWindowFlags wf = ImGuiWindowFlags_NoDocking;
		ImGuiViewport* vp = ImGui::GetMainViewport();
		ImGui::SetNextWindowPos(vp->Pos);
		ImGui::SetNextWindowSize(vp->Size);
		ImGui::SetNextWindowViewport(vp->ID);

		wf |= ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove;
		wf |= ImGuiWindowFlags_NoBringToFrontOnFocus | ImGuiWindowFlags_NoNavFocus;

		ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 0.0f);
		ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 0.0f);
		ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0, 0));

		ImGui::Begin("QuasarHub", &open, wf);

		ImGui::PopStyleVar(3);

		DrawHeader();

		ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(14, 14));
		ImGui::PushStyleColor(ImGuiCol_ChildBg, ImGui::GetStyleColorVec4(ImGuiCol_WindowBg));

		ImGui::BeginChild("Body", ImVec2(0, 0), false);

		ImGui::BeginChild("Sidebar", ImVec2(240, 0), true);
		DrawSidebar();
		ImGui::EndChild();

		ImGui::SameLine();

		ImGui::BeginChild("Main", ImVec2(0, 0), false);

		float detailsW = 360.0f;
		float availW = ImGui::GetContentRegionAvail().x;
		float contentW = std::max(0.0f, availW - detailsW - 12.0f);

		ImGui::BeginChild("Content", ImVec2(contentW, 0), false);

		switch (m_Page)
		{
		case Page::Home:      DrawHome(); break;
		case Page::Projects:  DrawProjects(); break;
		case Page::Templates: DrawTemplates(); break;
		case Page::Settings:  DrawSettings(); break;
		}

		ImGui::EndChild();
		ImGui::SameLine();

		ImGui::BeginChild("Details", ImVec2(0, 0), true);
		DrawDetailsPanel();
		ImGui::EndChild();

		ImGui::EndChild();

		ImGui::EndChild();

		ImGui::PopStyleColor();
		ImGui::PopStyleVar();

		DrawCreateModal();
		DrawOpenModal();

		ImGui::End();
	}

	void Launcher::DrawHeader()
	{
		ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(14, 10));
		ImGui::PushStyleColor(ImGuiCol_ChildBg, ImGui::GetStyleColorVec4(ImGuiCol_TitleBgActive));

		ImGui::BeginChild("Header", ImVec2(0, 62), false);

		ImGui::TextUnformatted("QUASAR HUB");
		ImGui::SameLine();
		ImGui::TextDisabled("  |  Editor Launcher");

		ImGui::SameLine();
		ImGui::SetCursorPosX(ImGui::GetWindowWidth() - 540);

		ImGui::SetNextItemWidth(260);
		ImGui::InputTextWithHint("##search", "Search projects...", &m_Search);

		ImGui::SameLine();
		ImGui::Checkbox("Pinned", &m_ShowOnlyPinned);

		ImGui::SameLine();
		if (ImGui::Button("Open..."))
			OpenOpenModal();

		ImGui::SameLine();
		if (ImGui::Button("Create"))
			OpenCreateModal(0);

		ImGui::EndChild();

		ImGui::PopStyleColor();
		ImGui::PopStyleVar();
	}

	void Launcher::DrawSidebar()
	{
		ImGui::TextDisabled("Navigation");
		ImGui::Spacing();

		auto nav = [&](const char* label, Page p)
			{
				bool selected = (m_Page == p);
				if (ImGui::Selectable(label, selected, 0, ImVec2(0, 34)))
					m_Page = p;
			};

		nav("Home", Page::Home);
		nav("Projects", Page::Projects);
		nav("Templates", Page::Templates);
		nav("Settings", Page::Settings);

		ImGui::Spacing();
		ImGui::Separator();
		ImGui::Spacing();

		ImGui::TextDisabled("Quick Actions");

		if (ImGui::Button("Continue", ImVec2(-1, 36)))
		{
			if (!m_Config.ProjectPath.empty() && RecentProjectsStore::ValidateProjectFolder(m_Config.ProjectPath))
			{
				std::string pn;
				RecentProjectsStore::ValidateProjectFolder(m_Config.ProjectPath, &pn);
				LaunchEditorWithProject(pn, m_Config.ProjectPath);
			}
		}

		if (ImGui::Button("Open folder...", ImVec2(-1, 36)))
			OpenOpenModal();

		if (ImGui::Button("New project...", ImVec2(-1, 36)))
		{
			OpenCreateModal(0);
		}

		ImGui::Spacing();
		ImGui::Separator();
		ImGui::Spacing();

		ImGui::TextDisabled("Tips");
		ImGui::TextWrapped("- Double click a card to open.\n- Right click for context menu.\n- Pin projects you use daily.");
	}

	void Launcher::DrawHome()
	{
		ImGui::TextUnformatted("Welcome back.");
		ImGui::Spacing();

		{
			ImGui::BeginChild("ContinueCard", ImVec2(0, 118), true);

			bool valid = !m_Config.ProjectPath.empty() && RecentProjectsStore::ValidateProjectFolder(m_Config.ProjectPath);
			ImGui::TextUnformatted("Continue");
			ImGui::Separator();

			if (valid)
			{
				ImGui::Text("Last project:");
				ImGui::SameLine();
				ImGui::TextUnformatted(m_Config.ProjectName.c_str());
				ImGui::TextDisabled("%s", m_Config.ProjectPath.c_str());

				if (ImGui::Button("Open last project", ImVec2(220, 36)))
					LaunchEditorWithProject(m_Config.ProjectName, m_Config.ProjectPath);
			}
			else
			{
				ImGui::TextDisabled("No valid last project found.");
				if (ImGui::Button("Open a project", ImVec2(220, 36)))
					OpenOpenModal();
			}

			ImGui::EndChild();
		}

		ImGui::Spacing();
		ImGui::SeparatorText("Recent Projects");
		ImGui::Spacing();

		float cardW = m_Config.Settings.CompactCards ? 260.0f : 300.0f;
		float cardH = m_Config.Settings.CompactCards ? 86.0f : 104.0f;
		DrawRecentGrid(cardW, cardH);
	}

	void Launcher::DrawProjects()
	{
		ImGui::SeparatorText("Projects");
		ImGui::Spacing();

		float cardW = m_Config.Settings.CompactCards ? 260.0f : 300.0f;
		float cardH = m_Config.Settings.CompactCards ? 86.0f : 104.0f;
		DrawRecentGrid(cardW, cardH);
	}

	void Launcher::DrawTemplates()
	{
		ImGui::SeparatorText("Templates");
		ImGui::TextDisabled("Pick a template, then create your project.");

		ImGui::Spacing();

		for (int i = 0; i < 4; i++)
		{
			ImGui::PushID(i);
			ImGui::BeginChild("tpl", ImVec2(0, 92), true);

			ImGui::TextUnformatted(m_Templates[i].Name);
			ImGui::SameLine();
			ImGui::TextDisabled("  [%s]", m_Templates[i].Tag);
			ImGui::TextWrapped("%s", m_Templates[i].Desc);

			ImGui::SameLine();
			float w = ImGui::GetContentRegionAvail().x;
			ImGui::SetCursorPosX(ImGui::GetCursorPosX() + std::max(0.0f, w - 160.0f));
			if (ImGui::Button("Use template", ImVec2(150, 34)))
				OpenCreateModal(m_Templates[i].Id);

			ImGui::EndChild();
			ImGui::PopID();

			ImGui::Spacing();
		}
	}

	void Launcher::DrawSettings()
	{
		ImGui::SeparatorText("Settings");

		bool dirty = false;

		dirty |= ImGui::Checkbox("Auto-open last project at startup", &m_Config.Settings.AutoOpenLastProject);
		dirty |= ImGui::Checkbox("Show missing projects in list", &m_Config.Settings.ShowMissingProjects);
		dirty |= ImGui::Checkbox("Compact cards", &m_Config.Settings.CompactCards);

		ImGui::Spacing();
		ImGui::Separator();
		ImGui::Spacing();

		if (dirty)
			m_Store.Save(m_Config);

		if (ImGui::Button("Clear recent list", ImVec2(200, 36)))
		{
			m_Config.RecentProjects.clear();
			m_SelectedIndex = -1;
			m_SelectedFolderSize.reset();
			m_Store.Save(m_Config);
		}
	}

	void Launcher::DrawRecentGrid(float cardW, float cardH)
	{
		std::vector<int> filtered;
		filtered.reserve(m_Config.RecentProjects.size());

		for (int i = 0; i < (int)m_Config.RecentProjects.size(); i++)
		{
			const auto& rp = m_Config.RecentProjects[i];

			if (!m_Config.Settings.ShowMissingProjects && IsMissing(rp.Path))
				continue;

			if (m_ShowOnlyPinned && !rp.Pinned)
				continue;

			if (!m_Search.empty())
			{
				std::string hay = rp.Name + " " + rp.Path;
				std::string hayL = ToLowerCopy(hay);
				std::string needle = ToLowerCopy(m_Search);
				if (hayL.find(needle) == std::string::npos)
					continue;
			}

			filtered.push_back(i);
		}

		if (filtered.empty())
		{
			ImGui::TextDisabled("No projects found.");
			return;
		}

		float avail = ImGui::GetContentRegionAvail().x;
		int cols = (int)(avail / (cardW + 12.0f));
		cols = std::max(1, cols);

		if (ImGui::BeginTable("recents", cols, ImGuiTableFlags_SizingFixedFit))
		{
			for (int k = 0; k < (int)filtered.size(); k++)
			{
				ImGui::TableNextColumn();

				int idx = filtered[k];
				const auto& rp = m_Config.RecentProjects[idx];

				bool selected = DrawProjectCard(idx, rp, cardW, cardH);
				if (selected)
				{
					m_SelectedIndex = idx;
					m_SelectedFolderSize.reset();
				}
			}
			ImGui::EndTable();
		}
	}

	bool Launcher::DrawProjectCard(int idx, const RecentProject& rp, float w, float h)
	{
		ImGui::PushID(idx);

		ImVec2 p = ImGui::GetCursorScreenPos();
		ImVec2 size(w, h);

		ImGui::InvisibleButton("card", size);

		bool hovered = ImGui::IsItemHovered();
		bool clicked = ImGui::IsItemClicked(ImGuiMouseButton_Left);
		bool dbl = ImGui::IsMouseDoubleClicked(ImGuiMouseButton_Left) && hovered;

		if (ImGui::BeginPopupContextItem("ctx"))
		{
			bool missing = IsMissing(rp.Path);

			if (ImGui::MenuItem("Open", nullptr, false, !missing))
			{
				std::string pn;
				RecentProjectsStore::ValidateProjectFolder(rp.Path, &pn);
				LaunchEditorWithProject(pn, rp.Path);
			}
			if (ImGui::MenuItem("Open in Explorer"))
				OpenInExplorer(rp.Path);

			ImGui::Separator();

			if (ImGui::MenuItem(rp.Pinned ? "Unpin" : "Pin"))
			{
				m_Store.SetPinned(m_Config, rp.Path, !rp.Pinned);
				SortRecents(m_Config.RecentProjects);
				m_Store.Save(m_Config);
			}

			if (ImGui::MenuItem("Remove from recents"))
			{
				m_Store.Remove(m_Config, rp.Path);
				if (m_SelectedIndex == idx) m_SelectedIndex = -1;
				SortRecents(m_Config.RecentProjects);
				m_Store.Save(m_Config);
			}

			ImGui::EndPopup();
		}

		auto* dl = ImGui::GetWindowDrawList();
		ImU32 bg = ImGui::GetColorU32(hovered ? ImGuiCol_FrameBgHovered : ImGuiCol_FrameBg);
		ImU32 border = ImGui::GetColorU32(ImGuiCol_Border);
		float rounding = 14.0f;

		dl->AddRectFilled(p, ImVec2(p.x + size.x, p.y + size.y), bg, rounding);
		dl->AddRect(p, ImVec2(p.x + size.x, p.y + size.y), border, rounding);

		ImU32 accent = ImGui::GetColorU32(ImVec4(0.55f, 0.56f, 0.98f, hovered ? 0.55f : 0.35f));
		dl->AddRectFilled(p, ImVec2(p.x + 6.0f, p.y + size.y), accent, rounding, ImDrawFlags_RoundCornersLeft);

		ImGui::SetCursorScreenPos(ImVec2(p.x + 14, p.y + 12));
		ImGui::BeginGroup();

		if (rp.Pinned) ImGui::TextUnformatted("X ");
		else ImGui::TextUnformatted("  ");
		ImGui::SameLine();

		bool missing = IsMissing(rp.Path);
		if (missing)
		{
			ImGui::TextUnformatted(rp.Name.c_str());
			ImGui::SameLine();
			ImGui::TextDisabled(" (Missing)");
		}
		else
		{
			ImGui::TextUnformatted(rp.Name.c_str());
		}

		ImGui::TextDisabled("%s", rp.Path.c_str());

		if (rp.LastOpenedUnix != 0)
		{
			ImGui::TextDisabled("Last opened: %s", HumanTimeAgo(rp.LastOpenedUnix).c_str());
		}

		ImGui::EndGroup();

		bool selected = clicked;

		if (dbl && !missing)
		{
			std::string pn;
			RecentProjectsStore::ValidateProjectFolder(rp.Path, &pn);
			LaunchEditorWithProject(pn, rp.Path);
		}

		ImGui::PopID();
		return selected;
	}

	void Launcher::DrawDetailsPanel()
	{
		ImGui::TextUnformatted("Project Details");
		ImGui::Separator();

		if (m_SelectedIndex < 0 || m_SelectedIndex >= (int)m_Config.RecentProjects.size())
		{
			ImGui::TextDisabled("Select a project card.");
			return;
		}

		const auto& rp = m_Config.RecentProjects[m_SelectedIndex];
		bool missing = IsMissing(rp.Path);

		ImGui::TextUnformatted(rp.Name.c_str());
		ImGui::TextDisabled("%s", rp.Path.c_str());

		ImGui::Spacing();

		if (rp.LastOpenedUnix)
		{
			ImGui::Text("Last opened");
			ImGui::SameLine();
			ImGui::TextDisabled("%s", HumanTimeAgo(rp.LastOpenedUnix).c_str());
		}

		if (!m_SelectedFolderSize.has_value() && !missing)
			m_SelectedFolderSize = TryComputeFolderSizeOnce(rp.Path);

		if (m_SelectedFolderSize.has_value())
		{
			ImGui::Text("Size");
			ImGui::SameLine();
			ImGui::TextDisabled("%s", BytesToNice(*m_SelectedFolderSize).c_str());
		}

		ImGui::Spacing();
		ImGui::Separator();
		ImGui::Spacing();

		if (ImGui::Button("Open", ImVec2(-1, 36)))
		{
			if (!missing)
			{
				std::string pn;
				RecentProjectsStore::ValidateProjectFolder(rp.Path, &pn);
				LaunchEditorWithProject(pn, rp.Path);
			}
		}
		if (missing)
			ImGui::TextDisabled("Project folder missing. Use Explorer or remove it.");

		if (ImGui::Button("Open in Explorer", ImVec2(-1, 36)))
			OpenInExplorer(rp.Path);

		if (ImGui::Button(rp.Pinned ? "Unpin" : "Pin", ImVec2(-1, 36)))
		{
			m_Store.SetPinned(m_Config, rp.Path, !rp.Pinned);
			SortRecents(m_Config.RecentProjects);
			m_Store.Save(m_Config);
		}

		if (ImGui::Button("Remove from recents", ImVec2(-1, 36)))
		{
			m_Store.Remove(m_Config, rp.Path);
			m_SelectedIndex = -1;
			m_SelectedFolderSize.reset();
			SortRecents(m_Config.RecentProjects);
			m_Store.Save(m_Config);
		}

		ImGui::Spacing();
		ImGui::Separator();
		ImGui::Spacing();

		ImGui::TextDisabled("Tip: right click a card for more options.");
	}

	void Launcher::OpenCreateModal(int templateId)
	{
		m_CreateModalOpen = true;
		m_NewProjectTemplate = templateId;
		m_LastError.clear();

		if (m_NewProjectName.empty())
			m_NewProjectName = "MyProject";

		if (m_NewProjectBasePath.empty())
			m_NewProjectBasePath = Utils::openFolder();
	}

	void Launcher::OpenOpenModal()
	{
		m_OpenModalOpen = true;
		if (m_OpenProjectPath.empty())
			m_OpenProjectPath = m_Config.ProjectPath.empty() ? "" : m_Config.ProjectPath;

		ImGui::OpenPopup("Open Project");
	}

	void Launcher::DrawCreateModal()
	{
		if (!m_CreateModalOpen)
			return;

		if (!ImGui::IsPopupOpen("Create Project", ImGuiPopupFlags_AnyPopupId))
			ImGui::OpenPopup("Create Project");

		if (ImGui::BeginPopupModal("Create Project", &m_CreateModalOpen, ImGuiWindowFlags_AlwaysAutoResize))
		{
			std::cout << "Drawing create modal..." << std::endl;
			ImGui::TextDisabled("Template");
			for (int i = 0; i < 4; i++)
			{
				ImGui::PushID(i);
				if (ImGui::RadioButton(m_Templates[i].Name, m_NewProjectTemplate == m_Templates[i].Id))
					m_NewProjectTemplate = m_Templates[i].Id;
				ImGui::PopID();
			}

			ImGui::Spacing();
			ImGui::Separator();
			ImGui::Spacing();

			ImGui::TextDisabled("Project name");
			ImGui::SetNextItemWidth(420);
			ImGui::InputText("##np_name", &m_NewProjectName);

			ImGui::TextDisabled("Base path");
			ImGui::SetNextItemWidth(360);
			ImGui::InputText("##np_path", &m_NewProjectBasePath);
			ImGui::SameLine();
			if (ImGui::Button("Browse"))
			{
				std::string chosen = Utils::openFolder();
				if (!chosen.empty())
					m_NewProjectBasePath = chosen;
			}

			std::string fullPath = m_NewProjectBasePath;
			if (!fullPath.empty() && !m_NewProjectName.empty())
				fullPath += "\\" + m_NewProjectName;

			ImGui::Spacing();
			ImGui::Text("Will create:");
			ImGui::TextDisabled("%s", fullPath.c_str());

			if (!m_LastError.empty())
			{
				ImGui::Spacing();
				ImGui::TextColored(ImVec4(1, 0.35f, 0.35f, 1), "%s", m_LastError.c_str());
			}

			ImGui::Spacing();

			bool canCreate = !m_NewProjectName.empty() && !m_NewProjectBasePath.empty();
			if (ImGui::Button("Create", ImVec2(140, 34)))
			{
				m_LastError.clear();

				if (!canCreate)
					m_LastError = "Please provide a name and a base path.";
				else if (std::filesystem::exists(fullPath))
					m_LastError = "Folder already exists.";
				else
				{
					if (CreateProjectOnDisk(m_NewProjectName, fullPath, m_NewProjectTemplate))
					{
						LaunchEditorWithProject(m_NewProjectName, fullPath);
					}
					else
					{
						m_LastError = "Failed to create project on disk.";
					}
				}
			}

			ImGui::SameLine();
			if (ImGui::Button("Cancel", ImVec2(140, 34)))
			{
				m_CreateModalOpen = false;
				m_LastError.clear();
				ImGui::CloseCurrentPopup();
			}

			ImGui::EndPopup();
		}
		else
		{
			m_CreateModalOpen = false;
		}
	}

	void Launcher::DrawOpenModal()
	{
		if (!m_OpenModalOpen)
			return;

		if (ImGui::BeginPopupModal("Open Project", nullptr, ImGuiWindowFlags_AlwaysAutoResize))
		{
			ImGui::TextDisabled("Project folder (must contain .ultprj)");
			ImGui::SetNextItemWidth(420);
			ImGui::InputText("##op_path", &m_OpenProjectPath);

			ImGui::SameLine();
			if (ImGui::Button("Browse"))
			{
				std::string chosen = Utils::openFolder();
				if (!chosen.empty())
					m_OpenProjectPath = chosen;
			}

			if (!m_LastError.empty())
			{
				ImGui::Spacing();
				ImGui::TextColored(ImVec4(1, 0.35f, 0.35f, 1), "%s", m_LastError.c_str());
			}

			ImGui::Spacing();

			if (ImGui::Button("Open", ImVec2(140, 34)))
			{
				m_LastError.clear();

				std::string pname;
				if (!RecentProjectsStore::ValidateProjectFolder(m_OpenProjectPath, &pname))
				{
					m_LastError = "Invalid project folder (missing .ultprj).";
				}
				else
				{
					LaunchEditorWithProject(pname, m_OpenProjectPath);
				}
			}

			ImGui::SameLine();
			if (ImGui::Button("Cancel", ImVec2(140, 34)))
			{
				m_OpenModalOpen = false;
				m_LastError.clear();
				ImGui::CloseCurrentPopup();
			}

			ImGui::EndPopup();
		}
		else
		{
			m_OpenModalOpen = false;
		}
	}

	bool Launcher::CreateProjectOnDisk(const std::string& projectName, const std::string& projectPath, int templateId)
	{
		try
		{
			std::filesystem::create_directories(projectPath);
			std::filesystem::create_directories(projectPath + "\\Assets");
			std::filesystem::create_directories(projectPath + "\\Scripts");
			std::filesystem::create_directories(projectPath + "\\.ultiris");

			switch (templateId)
			{
			case 1:
				std::filesystem::create_directories(projectPath + "\\Assets\\Scenes");
				std::filesystem::create_directories(projectPath + "\\Assets\\Textures");
				break;
			case 2:
				std::filesystem::create_directories(projectPath + "\\Assets\\Scenes");
				std::filesystem::create_directories(projectPath + "\\Assets\\Meshes");
				std::filesystem::create_directories(projectPath + "\\Assets\\Materials");
				break;
			case 3:
				std::filesystem::create_directories(projectPath + "\\Assets\\UI");
				std::filesystem::create_directories(projectPath + "\\Assets\\Scenes");
				break;
			default:
				break;
			}

			YAML::Emitter out;
			out << YAML::BeginMap;
			out << YAML::Key << "Project" << YAML::Value << projectName;
			out << YAML::Key << "Path" << YAML::Value << projectPath;
			out << YAML::EndMap;

			std::ofstream prj(projectPath + "\\" + projectName + ".ultprj");
			if (!prj.is_open())
				return false;
			prj << out.c_str();
			prj.close();

			{
				std::ofstream readme(projectPath + "\\README.txt");
				readme << "QuasarEngine project: " << projectName << "\n";
				readme << "Template: " << templateId << "\n";
			}

			m_Config.ProjectName = projectName;
			m_Config.ProjectPath = projectPath;

			m_Store.Touch(m_Config, projectName, projectPath);
			SortRecents(m_Config.RecentProjects);
			m_Store.Save(m_Config);

			return true;
		}
		catch (...)
		{
			return false;
		}
	}

	void Launcher::LaunchEditorWithProject(const std::string& projectName, const std::string& projectPath)
	{
		m_Config.ProjectName = projectName;
		m_Config.ProjectPath = projectPath;

		m_Store.Touch(m_Config, projectName, projectPath);
		SortRecents(m_Config.RecentProjects);
		m_Store.Save(m_Config);

		std::string exe = GetExePath();
		std::string cmd = "\"" + exe + "\" --project=\"" + projectPath + "\"";

#ifdef _WIN32
		STARTUPINFOA si = { sizeof(si) };
		PROCESS_INFORMATION pi;

		BOOL ok = CreateProcessA(
			NULL,
			cmd.data(),
			NULL, NULL,
			FALSE,
			DETACHED_PROCESS | CREATE_NEW_PROCESS_GROUP,
			NULL,
			NULL,
			&si,
			&pi
		);

		if (ok)
		{
			CloseHandle(pi.hProcess);
			CloseHandle(pi.hThread);
			Application::Get().Close();
		}
		else
		{
			MessageBoxA(NULL, "Échec du lancement de l'éditeur", "Erreur", MB_ICONERROR);
		}
#else
		// TODO: impl cross-platform
#endif
	}

	void Launcher::OpenInExplorer(const std::string& folderPath)
	{
#ifdef _WIN32
		ShellExecuteA(NULL, "open", folderPath.c_str(), NULL, NULL, SW_SHOWNORMAL);
#else
		(void)folderPath;
#endif
	}

	std::string Launcher::HumanTimeAgo(std::uint64_t unixSeconds)
	{
		std::uint64_t now = RecentProjectsStore::NowUnix();
		if (unixSeconds == 0 || unixSeconds > now) return "unknown";

		std::uint64_t d = now - unixSeconds;
		const std::uint64_t min = 60;
		const std::uint64_t hr = 3600;
		const std::uint64_t day = 86400;

		if (d < min) return "just now";
		if (d < hr)  return std::to_string(d / min) + " min ago";
		if (d < day) return std::to_string(d / hr) + " hours ago";
		return std::to_string(d / day) + " days ago";
	}

	std::string Launcher::BytesToNice(std::uint64_t bytes)
	{
		const double KB = 1024.0;
		const double MB = KB * 1024.0;
		const double GB = MB * 1024.0;

		char buf[64];
		if (bytes < (std::uint64_t)MB)
			snprintf(buf, sizeof(buf), "%.1f KB", bytes / KB);
		else if (bytes < (std::uint64_t)GB)
			snprintf(buf, sizeof(buf), "%.1f MB", bytes / MB);
		else
			snprintf(buf, sizeof(buf), "%.2f GB", bytes / GB);

		return buf;
	}

	std::optional<std::uint64_t> Launcher::TryComputeFolderSizeOnce(const std::string& folder)
	{
		try
		{
			std::uint64_t total = 0;
			for (auto& p : std::filesystem::recursive_directory_iterator(folder))
			{
				if (p.is_regular_file())
					total += (std::uint64_t)p.file_size();
			}
			return total;
		}
		catch (...)
		{
			return std::nullopt;
		}
	}
}
