#include "Editor.h"

#include <fstream>

#include <glm/glm.hpp>
#include <glm/gtc/type_ptr.hpp>

#include "imgui/imgui.h"
#include "imgui/imgui_internal.h"
#include "ImGuizmo.h"

#include "QuasarEngine/Core/MouseCodes.h"
#include "QuasarEngine/Physic/PhysicEngine.h"

#include "QuasarEngine/Resources/ResourceManager.h"

#include "QuasarEngine/Asset/AssetManager.h"

#include <QuasarEngine/Memory/MemoryTracker.h>

#include <yaml-cpp/yaml.h>

#include <QuasarEngine/Scripting/QMM/VM.h>

#include <QuasarEngine/Physic/PhysXTest.h>

namespace QuasarEngine
{
	Editor::Editor(const EditorSpecification& spec)
		: Layer("Editor"),
		m_Specification(spec),
		m_EditorCamera(std::make_unique<EditorCamera>(glm::vec3(0.0f, 0.0f, 0.0f)))
	{
		Application::Get().MaximizeWindow(false);

		std::cout << "  ___                               _____             _" << std::endl;
		std::cout << " / _ \\ _   _  __ _ ___  __ _ _ __  | ____|_ __   __ _(_)_ __   ___" << std::endl;
		std::cout << "| | | | | | |/ _` / __|/ _` | '__| |  _| | '_ \\ / _` | | '_ \\ / _ \\" << std::endl;
		std::cout << "| |_| | |_| | (_| \\__ \\ (_| | |    | |___| | | | (_| | | | | |  __/" << std::endl;
		std::cout << " \\__\\_\\\\__,_|\\__,_|___/\\__,_|_|    |_____|_| |_|\\__, |_|_| |_|\\___|" << std::endl;
		std::cout << "                                                |___ /" << std::endl;

	}

	void Editor::OnAttach()
	{
		InitImGuiStyle();

		PhysicEngine::Init();

		/*AssetToLoad asset;
		asset.type = TEXTURE;
		asset.id = "Assets/Icons/no_texture.png";
		Renderer::m_SceneData.m_AssetManager->loadAsset(asset);*/

		//m_EditorCamera->m_CameraFocus = true;

		m_AssetImporter = std::make_unique<AssetImporter>(m_Specification.ProjectPath);

		m_EntityPropertiePanel = std::make_unique<EntityPropertiePanel>(m_Specification.ProjectPath);
		m_SceneHierarchy = std::make_unique<SceneHierarchy>();
		m_ContentBrowserPanel = std::make_unique<ContentBrowserPanel>(m_Specification.ProjectPath, m_AssetImporter.get());
		m_EditorViewport = std::make_unique<EditorViewport>();
		m_Viewport = std::make_unique<Viewport>();
		m_NodeEditor = std::make_unique<NodeEditor>();
		m_AnimationEditorPanel = std::make_unique<AnimationEditorPanel>();
		//m_CodeEditor = std::make_unique<CodeEditor>();
		m_HeightMapEditor = std::make_unique<HeightMapEditor>();

		m_SceneManager = std::make_unique<SceneManager>(m_Specification.ProjectPath);
		m_SceneManager->createNewScene();

		Renderer::BeginScene(m_SceneManager->GetActiveScene());

		/*MaterialSpecification mat;
		mat.AlbedoTexture = "Assets/Textures/white_texture.jpg";
		QuasarEngine::SceneManager::ModelToLoad modelToLoad;
		modelToLoad.path = "C:\\Users\\rouff\\Documents\\Ultiris Projects\\CallOf\\Assets\\Models\\BackPackFusion.obj";
		modelToLoad.material = mat;
		m_SceneManager->LoadModel(modelToLoad);*/

		std::filesystem::path base_path = m_Specification.ProjectPath + "\\Assets";
		SetupAssets(base_path);

		VM vm;

		vm.registerFunction("create_entity", [](const std::vector<Value::Prim>& args)->Value::Prim {
			std::string name = args.size() > 0 && std::holds_alternative<std::string>(args[0]) ? std::get<std::string>(args[0]) : "Unnamed";
			static double nextId = 1; double id = nextId++;
			std::cout << "[ENGINE] Create entity '" << name << "' -> id=" << id << "\n";
			return id;
			});

		const std::string code = R"(
			// =====================
			// Tests de base
			// =====================
			print("== scalaires ==");
			let a = 1_234.5e-1;
			let b = 2;
			let c = a + b * 3 - 4 / 2 + 7 % 3;
			print("a=", a, " b=", b, " c=", c);

			print("== chaînes & escapes ==");
			let s = "hello\n\tworld \"quote\" \\ backslash \u263A";
			print(s);
			print("len(s)=", len(s), " substr(s,7,5)=", substr(s, 7, 5));
			print("str(3.14)=", str(3.14), " str(true)=", str(true));

			// =====================
			// Logique & égalité typée
			// =====================
			print("== logique / égalité ==");
			let t = true; let f = false; let z = nil;
			print("truthy(true)=", t, " truthy(false)=", f, " truthy(nil)=", z);
			print("1 == 1     ->", 1 == 1);
			print("1 == \"1\"  ->", 1 == "1");
			print("true != false ->", true != false);

			fn ping(msg) { print(msg); return true; }
			fn pong(msg) { print(msg); return false; }
			print("OR short-circuit (true || ...):");
			print( (ping("ping called") || ping("NE DOIT PAS S'AFFICHER")) );
			print("AND short-circuit (false && ...):");
			print( (pong("pong called") && ping("NE DOIT PAS S'AFFICHER")) );

			// =====================
			// Contrôle de flux
			// =====================
			print("== if/else & while ==");
			let n = 5;
			if (n > 3) { print("n > 3"); } else { print("n <= 3"); }

			let fact = 1;
			let i = 1;
			while (i <= n) { fact = fact * i; i = i + 1; }
			print("fact(", n, ") = ", fact);

			// =====================
			// Fonctions nommées + return
			// =====================
			print("== fonctions nommées ==");
			fn add(x, y) { return x + y; }
			fn fib(k) {
				if (k <= 1) { return k; }
				return fib(k-1) + fib(k-2);
			}
			print("add(2,3)=", add(2,3), " fib(10)=", fib(10));

			// =====================
			// [FNEXPR] Fonctions anonymes & closures
			// =====================
			print("== fonctions anonymes & closures ==");
			fn makeAdder(a) {
				return fn (b) { return a + b; };
			}
			let add10 = makeAdder(10);
			print("add10(7)=", add10(7));

			print("square(6) via (fn(x){...})(6): ", (fn (x) { return x * x; })(6));

			// =====================
			// Stdlib math / random / clock
			// =====================
			print("== stdlib math/random/clock ==");
			print("sin(0)=", sin(0), " cos(0)=", cos(0), " sqrt(9)=", sqrt(9));
			let r = rand();
			print("rand()=", r);
			let t0 = clock();
			let spin = 0;
			while (spin < 10000) { spin = spin + 1; }
			let t1 = clock();
			print("clock delta ~", t1 - t0);

			// =====================
			// Nil & vérité
			// =====================
			print("== nil & vérité ==");
			let maybe = nil;
			if (maybe) { print("nil est truthy ?! erreur");
				}
			else { print("nil est falsy ok"); }

			// =====================
			// Erreurs évitées (division/mod zéro) — exemples sûrs
			// =====================
			print("== division/mod sûrs ==");
			print("10 / 2 = ", 10 / 2);
			print("10 % 3 = ", 10 % 3);

			// =====================
			// Création d'entités (bindings natifs)
			// =====================
			print("== test create_entity ==");

			let id1 = create_entity("Player");
			let id2 = create_entity("Enemy");
			let id3 = create_entity();

			print("ids:", id1, id2, id3);

			// Fin
			print("== DONE ==");
		)";

		try { vm.eval(code); }
		catch (const QError& e) { std::cerr << "Q-- Error: " << e.what() << "\n"; }

		PhysXTest test;

		for (int i = 0; i < 300; i++)
		{
			test.StepSimulation(1.0f / 60.0f);
		}
	}

	void Editor::SetupAssets(const std::filesystem::path& chemin) {
		for (const auto& entry : std::filesystem::directory_iterator(chemin)) {
			if (entry.is_directory())
			{
				SetupAssets(entry.path());
			}
			else
			{
				AssetType type = Renderer::m_SceneData.m_AssetManager->getAssetType(entry.path().string());
				if (type != NONE)
				{
					Renderer::m_SceneData.m_AssetManager->registerAsset(entry.path().string(), type);
				}
			}
		}
	}

	void Editor::OnDetach()
	{
		PhysicEngine::Shutdown();

		m_SceneManager.reset();
		m_EntityPropertiePanel.reset();
		m_SceneHierarchy.reset();
		m_ContentBrowserPanel.reset();
		m_EditorViewport.reset();
		m_Viewport.reset();
		m_NodeEditor.reset();
		m_AnimationEditorPanel.reset();
		//m_CodeEditor.reset();
		m_HeightMapEditor.reset();
	}

	void Editor::OnUpdate(double dt)
	{
		Input::Update();

		m_ContentBrowserPanel->Update();
		m_EditorCamera->Update();
		m_SceneManager->Update(dt);
		m_EditorViewport->Update(*m_EditorCamera);
		m_Viewport->Update(m_SceneManager->GetActiveScene());
		m_AnimationEditorPanel->Update(dt);
		m_HeightMapEditor->Update();

		if (Input::IsKeyPressed(Key::LeftControl))
		{
			if (Input::IsKeyPressed(Key::S))
			{
				m_SceneManager->SaveScene();
			}
		}

		auto& tracker = MemoryTracker::instance();
		tracker.Update(dt);
	}

	void Editor::OnRender()
	{
		m_EditorViewport->Render(m_SceneManager->GetActiveScene(), *m_EditorCamera);
		m_Viewport->Render(m_SceneManager->GetActiveScene());
	}

	void Editor::DrawStatsWindow()
	{
		static FrameTimeHistory frame_history;

		const ApplicationInfos& infos = Application::Get().GetAppInfos();
		auto& tracker = MemoryTracker::instance();

		ImGui::Begin("Performance & Memory Tracker");

		DrawFrameStats(infos, frame_history);

		ImGui::Text("Total Allocated: %.2f MB", tracker.GetTotalAllocated() / (1024.0f * 1024.0f));
		ImGui::Text("Total Freed:     %.2f MB", tracker.GetTotalFreed() / (1024.0f * 1024.0f));
		ImGui::Text("Current Usage:   %.2f MB", tracker.GetCurrentUsage() / (1024.0f * 1024.0f));
		ImGui::PlotLines("Memory Usage", tracker.GetHistory().data(), tracker.GetHistory().size(), 0, nullptr, 0.0f, FLT_MAX, ImVec2(-1, 80));

		ImGui::End();
	}

	void Editor::DrawFrameStats(const ApplicationInfos& infos, FrameTimeHistory& history)
	{
		static double last_push = 0.0;
		double now = ImGui::GetTime();

		if (now - last_push > 1.0)
		{
			history.Push(infos);
			last_push = now;
		}

		ImGui::Text("FPS: %.1f | Frame Time: %.2f ms", infos.app_fps, 1000.0 / std::max(1.0, infos.app_fps));
		ImGui::Separator();

		ImVec2 plot_size = ImVec2(ImGui::GetContentRegionAvail().x, 100);
		ImDrawList* draw_list = ImGui::GetWindowDrawList();
		ImVec2 pos = ImGui::GetCursorScreenPos();

		int size = history.Size();
		if (size < 2) {
			ImGui::Dummy(plot_size);
			return;
		}

		constexpr int RECENT_COUNT = 4;
		float max_recent = 0.0f;
		int actual_count = std::min(size, RECENT_COUNT);
		int start = history.filled ? history.index : 0;

		for (int i = 0; i < actual_count; ++i)
		{
			int idx = (start + size - actual_count + i) % FRAME_HISTORY_SIZE;
			float total = history.begin[idx] + history.update[idx] + history.render[idx] + history.end[idx]
				+ history.event[idx] + history.asset[idx] + history.imgui[idx];
			if (total > max_recent) max_recent = total;
		}
		if (max_recent < 1.5f) max_recent = 1.5f;

		float max_v = max_recent / 0.95f;

		if (max_v < 3.0f)  max_v = 3.0f;
		if (max_v > 50.0f) max_v = 50.0f;

		ImU32 colors[] = {
			IM_COL32(200, 100, 255, 255), // begin
			IM_COL32(150, 230, 120, 255), // update
			IM_COL32(80, 170, 255, 255),  // render
			IM_COL32(255, 210, 120, 255), // end
			IM_COL32(255, 100, 100, 255), // event
			IM_COL32(100, 210, 180, 255), // asset
			IM_COL32(255, 110, 220, 255)  // imgui
		};
		const char* labels[] = {
			"Begin", "Update", "Render", "End", "Event", "Asset", "ImGui"
		};
		float* curves[] = {
			history.begin, history.update, history.render, history.end,
			history.event, history.asset, history.imgui
		};
		int n_curves = 7;

		for (int c = 0; c < n_curves; ++c)
		{
			for (int i = 1; i < size; ++i)
			{
				int idx0 = (start + i - 1) % FRAME_HISTORY_SIZE;
				int idx1 = (start + i) % FRAME_HISTORY_SIZE;

				float x0 = pos.x + ((i - 1) / float(size - 1)) * plot_size.x;
				float x1 = pos.x + (i / float(size - 1)) * plot_size.x;

				float y0 = pos.y + plot_size.y - (curves[c][idx0] / max_v) * plot_size.y;
				float y1 = pos.y + plot_size.y - (curves[c][idx1] / max_v) * plot_size.y;
				draw_list->AddLine(ImVec2(x0, y0), ImVec2(x1, y1), colors[c], 2.0f);
			}
		}

		draw_list->AddRect(pos, ImVec2(pos.x + plot_size.x, pos.y + plot_size.y), IM_COL32(180, 180, 180, 100));
		ImGui::Dummy(plot_size);

		auto ColorU32ToVec4 = [](ImU32 color) -> ImVec4 {
			float r = ((color >> IM_COL32_R_SHIFT) & 0xFF) / 255.0f;
			float g = ((color >> IM_COL32_G_SHIFT) & 0xFF) / 255.0f;
			float b = ((color >> IM_COL32_B_SHIFT) & 0xFF) / 255.0f;
			float a = ((color >> IM_COL32_A_SHIFT) & 0xFF) / 255.0f;
			return ImVec4(r, g, b, a);
			};

		ImGui::BeginGroup();
		for (int c = 0; c < n_curves; ++c)
		{
			ImGui::SameLine();
			ImGui::ColorButton(labels[c], ColorU32ToVec4(colors[c]),
				ImGuiColorEditFlags_NoTooltip | ImGuiColorEditFlags_NoDragDrop, ImVec2(8, 8));
			ImGui::SameLine(0, 2);
			ImGui::Text("%s", labels[c]);
			ImGui::SameLine(0, 12);
		}
		ImGui::EndGroup();
		ImGui::Separator();
	}

	void Editor::OnGuiRender()
	{
		/*Renderer::BeginScene(m_SceneManager->GetActiveScene());
		Renderer::Render(*m_EditorCamera.get());
		Renderer::EndScene();*/

		static bool dockspaceOpen = true;
		static bool opt_fullscreen = true;
		static bool opt_padding = false;
		static ImGuiDockNodeFlags dockspace_flags = ImGuiDockNodeFlags_None;

		ImGuiWindowFlags window_flags = ImGuiWindowFlags_MenuBar | ImGuiWindowFlags_NoDocking;
		if (opt_fullscreen)
		{
			ImGuiViewport* viewport = ImGui::GetMainViewport();
			ImGui::SetNextWindowPos(viewport->WorkPos);
			ImGui::SetNextWindowSize(viewport->WorkSize);
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

			bool imgui_ini_exist = std::filesystem::exists(std::filesystem::current_path().generic_string() + "/imgui.ini");
			if (!imgui_ini_exist)
			{
				ImGui::DockBuilderRemoveNode(dockspace_id);
				ImGui::DockBuilderAddNode(dockspace_id, dockspace_flags | ImGuiDockNodeFlags_DockSpace);
				ImGui::DockBuilderSetNodeSize(dockspace_id, ImGui::GetMainViewport()->Size);

				ImGuiID dock_main_id = dockspace_id;
				ImGuiID dock_id_left = ImGui::DockBuilderSplitNode(dock_main_id, ImGuiDir_Left, 0.20f, nullptr, &dock_main_id);
				ImGuiID dock_id_right = ImGui::DockBuilderSplitNode(dock_main_id, ImGuiDir_Right, 0.20f, nullptr, &dock_main_id);
				ImGuiID dock_id_bottom = ImGui::DockBuilderSplitNode(dock_main_id, ImGuiDir_Down, 0.20f, nullptr, &dock_main_id);
				ImGuiID dock_id_inspector = ImGui::DockBuilderSplitNode(dock_id_right, ImGuiDir_Down, 0.20f, nullptr, &dock_id_right);

				ImGui::DockBuilderDockWindow("Viewport", dock_main_id);
				ImGui::DockBuilderDockWindow("Editor", dock_main_id);
				ImGui::DockBuilderDockWindow("Content Browser", dock_id_right);
				ImGui::DockBuilderDockWindow("Scene", dock_id_left);
				ImGui::DockBuilderDockWindow("Inspector", dock_id_right);
				ImGui::DockBuilderDockWindow("Editor infos", dock_id_inspector);

				ImGui::DockBuilderFinish(dockspace_id);
			}
		}

		bool hasProject = true;

		if (ImGui::BeginMenuBar())
		{
			if (ImGui::BeginMenu("Project"))
			{
				if (ImGui::MenuItem("Quit"))
					QuasarEngine::Application::Get().Close();

				ImGui::EndMenu();
			}

			if (ImGui::BeginMenu("Asset"))
			{
				if (ImGui::MenuItem("Import"))
				{
					m_AssetImporter->ImportAsset();
				}

				ImGui::EndMenu();
			}

			if (ImGui::BeginMenu("Scene", hasProject))
			{
				if (ImGui::MenuItem("New scene"))
				{
					m_SceneHierarchy->m_SelectedEntity = Entity::Null();
					m_SceneManager->createNewScene();
				}

				if (ImGui::MenuItem("Save scene"))
				{
					m_SceneManager->SaveScene();
				}

				if (ImGui::MenuItem("Load scene"))
				{
					m_SceneHierarchy->m_SelectedEntity = Entity::Null();
					m_SceneManager->LoadScene();
				}
				ImGui::EndMenu();
			}

			if (ImGui::BeginMenu("Script", hasProject))
			{
				if (ImGui::MenuItem("Build"))
				{
					
				}
				ImGui::EndMenu();
			}

			if (ImGui::BeginMenu("Runtime", hasProject))
			{
				if (ImGui::MenuItem("Start scene"))
				{
					m_SceneManager->SaveScene();
					m_SceneManager->GetActiveScene().OnRuntimeStart();
				}
				if (ImGui::MenuItem("Stop scene"))
				{					
					m_SceneManager->GetActiveScene().OnRuntimeStop();
					m_SceneManager->ReloadScene(m_SceneManager->GetSceneObject().GetPath());

				}
				ImGui::EndMenu();
			}

			if (ImGui::BeginMenu("Create", hasProject))
			{
				if (ImGui::MenuItem("Create new GameObject"))
				{
					Entity temp = m_SceneManager->GetActiveScene().CreateEntity("GameObject");
				}

				if (ImGui::MenuItem("Create new Cube")) m_SceneManager->AddCube();
				if (ImGui::MenuItem("Create new Sphere")) m_SceneManager->AddSphere();
				if (ImGui::MenuItem("Create new UV Sphere")) m_SceneManager->AddUVSphere();
				if (ImGui::MenuItem("Create new Plane")) m_SceneManager->AddPlane();;
				ImGui::EndMenu();
			}

			if (ImGui::BeginMenu("Options"))
			{
				if (ImGui::MenuItem("Preference"))
				{
					m_optionMenu = true;
					menu_state.set(MenuType::OPTION_MENU);
				}
				//ImGui::MenuItem("Fullscreen", NULL, &opt_fullscreen);
				//ImGui::MenuItem("Padding", NULL, &opt_padding);
				ImGui::EndMenu();
			}

			ImGui::EndMenuBar();
		}

		m_Viewport->OnImGuiRender(m_SceneManager->GetActiveScene());
		m_EditorViewport->OnImGuiRender(*m_EditorCamera, *m_SceneManager, *m_SceneHierarchy);
		m_EntityPropertiePanel->OnImGuiRender(m_SceneManager->GetActiveScene(), *m_SceneHierarchy);
		m_SceneHierarchy->OnImGuiRender(m_SceneManager->GetActiveScene());
		m_ContentBrowserPanel->OnImGuiRender();
		//m_CodeEditor->OnImGuiRender();
		m_AnimationEditorPanel->OnImGuiRender();
		m_NodeEditor->OnImGuiRender();
		m_HeightMapEditor->OnImGuiRender();

		OptionMenu();

		DrawStatsWindow();

		ImGui::Begin("Latences");

		ImGui::Text(std::string("Update: " + std::to_string(Application::Get().GetAppInfos().update_latency)).c_str());
		ImGui::Text(std::string("Render: " + std::to_string(Application::Get().GetAppInfos().render_latency)).c_str());
		ImGui::Text(std::string("Event: " + std::to_string(Application::Get().GetAppInfos().event_latency)).c_str());
		ImGui::Text(std::string("Asset: " + std::to_string(Application::Get().GetAppInfos().asset_latency)).c_str());
		ImGui::Text(std::string("ImGui: " + std::to_string(Application::Get().GetAppInfos().imgui_latency)).c_str());

		ImGui::End();

		ImGui::End();
	}

	void Editor::OnEvent(Event& e)
	{
		m_EditorCamera->OnEvent(e);

		EventDispatcher dispatcher(e);
		dispatcher.Dispatch<MouseButtonPressedEvent>(std::bind(&Editor::OnMouseButtonPressed, this, std::placeholders::_1));
		dispatcher.Dispatch<WindowResizeEvent>(std::bind(&Editor::OnWindowResized, this, std::placeholders::_1));
	}

	bool Editor::OnMouseButtonPressed(MouseButtonPressedEvent& e)
	{
		if (ImGuizmo::IsOver())
			return false;

		if (!m_EditorViewport->IsViewportHovered())
			return false;

		if (e.GetMouseButton() == Mouse::Button0)
		{
			//m_SceneHierarchy->m_SelectedEntity = m_EditorViewport->GetHoveredEntity();
			return true;
		}

		return false;
	}

	bool Editor::OnWindowResized(WindowResizeEvent& e)
	{
		//m_EditorCamera->OnResize(e.GetWidth(), e.GetHeight());

		return false;
	}

	void Editor::InitImGuiStyle()
	{
		m_ImGuiColor = {
			ImGuiCol_WindowBg,
			ImGuiCol_Header,
			ImGuiCol_HeaderHovered,
			ImGuiCol_HeaderActive,
			ImGuiCol_Button,
			ImGuiCol_ButtonHovered,
			ImGuiCol_ButtonActive,
			ImGuiCol_FrameBg,
			ImGuiCol_FrameBgHovered,
			ImGuiCol_FrameBgActive,
			ImGuiCol_Tab,
			ImGuiCol_TabHovered,
			ImGuiCol_TabActive,
			ImGuiCol_TabUnfocused,
			ImGuiCol_TabUnfocusedActive,
			ImGuiCol_TitleBg,
			ImGuiCol_TitleBgActive,
			ImGuiCol_TitleBgCollapsed,
			ImGuiCol_Border
		};

		m_ThemeName = {
			"WindowBg",
			"Header",
			"HeaderHovered",
			"HeaderActive",
			"Button",
			"ButtonHovered",
			"ButtonActive",
			"FrameBg",
			"FrameBgHovered",
			"FrameBgActive",
			"Tab",
			"TabHovered",
			"TabActive",
			"TabUnfocused",
			"TabUnfocusedActive",
			"TitleBg",
			"TitleBgActive",
			"TitleBgCollapsed",
			"Border"
		};

		std::ifstream fin(std::filesystem::current_path().generic_string() + "/theme.ini");
		if (fin.is_open())
		{
			YAML::Node data = YAML::Load(fin);
			for (int temp = 0; temp < m_ImGuiColor.size(); temp++)
			{
				m_ThemeColor[m_ImGuiColor[temp]] = { data[m_ThemeName[temp]]["x"].as<float>(), data[m_ThemeName[temp]]["y"].as<float>(), data[m_ThemeName[temp]]["z"].as<float>(), data[m_ThemeName[temp]]["w"].as<float>() };
				ImGuiStyle* style = &ImGui::GetStyle();
				ImVec4* colors = style->Colors;
				colors[m_ImGuiColor[temp]] = ImVec4(m_ThemeColor[m_ImGuiColor[temp]].x, m_ThemeColor[m_ImGuiColor[temp]].y, m_ThemeColor[m_ImGuiColor[temp]].z, m_ThemeColor[m_ImGuiColor[temp]].w);
			}
			fin.close();
		}
	}

	void Editor::OptionMenu()
	{
		if (menu_state[OPTION_MENU])
		{
			ImGui::Begin("Preference");
			{
				ImGui::Columns(2);
				ImGui::SetColumnOffset(1, 230);

				if (ImGui::Button("Theme", ImVec2(230 - 15, 39)))
					m_optionTab = 0;

				if (ImGui::Button("Save", ImVec2(230 - 15, 39)))
				{
					YAML::Emitter out;
					out << YAML::BeginMap;
					for (int temp = 0; temp < m_ImGuiColor.size(); temp++)
					{
						out << m_ThemeName[temp] << YAML::BeginMap;
						out << "x" << m_ThemeColor[m_ImGuiColor[temp]].x;
						out << "y" << m_ThemeColor[m_ImGuiColor[temp]].y;
						out << "z" << m_ThemeColor[m_ImGuiColor[temp]].z;
						out << "w" << m_ThemeColor[m_ImGuiColor[temp]].w;
						out << YAML::EndMap;
					}
					out << YAML::EndMap;
					std::ofstream fout(std::filesystem::current_path().generic_string() + "/theme.ini");
					fout << out.c_str();
					fout.close();
				}

				if (ImGui::Button("Close", ImVec2(230 - 15, 39)))
					menu_state.reset(MenuType::OPTION_MENU);
			}

			ImGui::NextColumn();

			{
				if (ImGui::Button("Modern Dark", ImVec2(230 - 70, 29)))
					ImGui::StyleColorsModernDark();
				ImGui::SameLine();
				if (ImGui::Button("Real Dark Bis", ImVec2(230 - 70, 29)))
					ImGui::StyleColorsModernDarkBis();
				ImGui::SameLine();
				if (ImGui::Button("Real Dark", ImVec2(230 - 70, 29)))
					ImGui::StyleColorsRealDark();
				ImGui::SameLine();
				if (ImGui::Button("Dark", ImVec2(230 - 70, 29)))
					ImGui::StyleColorsDark();
				ImGui::SameLine();
				if (ImGui::Button("Classic", ImVec2(230 - 70, 29)))
					ImGui::StyleColorsClassic();
				ImGui::SameLine();
				if (ImGui::Button("Modern Light", ImVec2(230 - 70, 29)))
					ImGui::StyleColorsModernLight();
				ImGui::SameLine();
				if (ImGui::Button("Light", ImVec2(230 - 70, 29)))
					ImGui::StyleColorsLight();

				ImGuiStyle* style = &ImGui::GetStyle();
				ImVec4* colors = style->Colors;

				for (int temp1 = 0; temp1 < m_ImGuiColor.size(); temp1++)
				{
					m_ThemeColor[m_ImGuiColor[temp1]] = { colors[m_ImGuiColor[temp1]].x, colors[m_ImGuiColor[temp1]].y, colors[m_ImGuiColor[temp1]].z, colors[m_ImGuiColor[temp1]].w };
				}

				for (int temp2 = 0; temp2 < m_ImGuiColor.size(); temp2++)
				{
					std::string temp("##" + std::string(m_ThemeName[temp2]));
					ImGui::Text(m_ThemeName[temp2]); ImGui::SameLine(); ImGui::ColorEdit4(temp.c_str(), glm::value_ptr(m_ThemeColor[m_ImGuiColor[temp2]]));
				}

				for (int temp3 = 0; temp3 < m_ImGuiColor.size(); temp3++)
				{
					colors[m_ImGuiColor[temp3]] = ImVec4(m_ThemeColor[m_ImGuiColor[temp3]].x, m_ThemeColor[m_ImGuiColor[temp3]].y, m_ThemeColor[m_ImGuiColor[temp3]].z, m_ThemeColor[m_ImGuiColor[temp3]].w);
				}
			}

			ImGui::End();
		}
	}
}
