#include "Editor.h"

#include <fstream>

#include <glm/glm.hpp>
#include <glm/gtc/type_ptr.hpp>

#include "imgui/imgui.h"
#include "imgui/imgui_internal.h"
#include "ImGuizmo.h"

#include "QuasarEngine/Core/Logger.h"

#include "QuasarEngine/Core/MouseCodes.h"
#include "QuasarEngine/Physic/PhysicEngine.h"

#include "QuasarEngine/Resources/ResourceManager.h"

#include "QuasarEngine/Asset/AssetManager.h"

#include <QuasarEngine/Memory/MemoryTracker.h>

#include <yaml-cpp/yaml.h>

#include <QuasarEngine/Scripting/QMM/VM.h>

#include <QuasarEngine/Physic/PhysXTest.h>

#ifndef QE_PROFILE_APP_TIMERS
#if !defined(NDEBUG)
#define QE_PROFILE_APP_TIMERS 1
#else
#define QE_PROFILE_APP_TIMERS 1
#endif
#endif

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

		Logger::initUtf8Console();

		PhysicEngine::Init();

		m_AssetImporter = std::make_unique<AssetImporter>(m_Specification.ProjectPath);

		m_EntityPropertiePanel = std::make_unique<EntityPropertiePanel>(m_Specification.ProjectPath);
		m_SceneHierarchy = std::make_unique<SceneHierarchy>();
		m_ContentBrowserPanel = std::make_unique<ContentBrowserPanel>(m_Specification.ProjectPath, m_AssetImporter.get());
		m_EditorViewport = std::make_unique<EditorViewport>();
		m_Viewport = std::make_unique<Viewport>();
		//m_NodeEditor = std::make_unique<NodeEditor>();
		//m_AnimationEditorPanel = std::make_unique<AnimationEditorPanel>();
		//m_HeightMapEditor = std::make_unique<HeightMapEditor>();
		//m_UserInterfaceEditor = std::make_unique<UserInterfaceEditor>();

		m_SceneManager = std::make_unique<SceneManager>(m_Specification.ProjectPath);
		m_SceneManager->createNewScene();

		Renderer::BeginScene(m_SceneManager->GetActiveScene());

		std::filesystem::path base_path = m_Specification.ProjectPath + "\\Assets";
		SetupAssets(base_path);

		/*VM vm;

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

		using QuasarEngine::Logger;
		using L = Logger::Level;

		const bool TEST_SINKS = false;
		const bool RUN_ASSERT = false;
		const int  THREADS = 3;
		const int  MSGS_PER_THR = 60;

		const L    oldMin = Logger::minLevel();
		const bool oldColors = Logger::colorsEnabled();
		const bool oldAbort = Logger::abortOnFatal();

		Logger::setAbortOnFatal(false);
		Q_DEBUG("SelfTest: DEBUG");
		Q_INFO("SelfTest: INFO");
		Q_WARNING("SelfTest: WARNING");
		Q_ERROR("SelfTest: ERROR");
		Q_FATAL("SelfTest: FATAL (ne doit pas abort)");

		Logger::setMinLevel(L::Warning);
		Q_INFO("NE DOIT PAS APPARAITRE (min=Warning)");
		Q_WARNING("OK (Warning passe)");
		Q_ERROR("OK (Error passe)");
		Logger::setMinLevel(L::Debug);

		Logger::enableColors(false);
		Q_INFO("Couleurs OFF");
		Logger::enableColors(true);
		Q_INFO("Couleurs ON");

		Logger::apiInfo("Vulkan", {
			{"Vendor",   "0.0.0.1"},
			{"Renderer", "0.0.0.1"},
			{"Version",  "0.0.0.1"}
			});

#if defined(__cpp_lib_format) && (__cpp_lib_format >= 201907L)
		Q_INFOF("Formatted: {} + {} = {}", 2, 2, 2 + 2);
		Q_WARNF("Formatted float ~ {}", 3.14159);
#else
		Q_INFO("C++20 <format> indisponible: Q_*F(...) sont no-op");
#endif

		{
			std::vector<std::thread> pool;
			for (int t = 0; t < THREADS; ++t) {
				pool.emplace_back([t, MSGS_PER_THR] {
					for (int i = 0; i < MSGS_PER_THR; ++i) {
						Q_DEBUG(std::string("T") + std::to_string(t) + " -> " + std::to_string(i));
						if ((i % 20) == 0) std::this_thread::sleep_for(std::chrono::milliseconds(1));
					}
					});
			}
			for (auto& th : pool) th.join();
			Q_INFO("Multithread OK");
		}

		if (TEST_SINKS) {
			Logger::addFileSink("logger_selftest.log", true);
			Q_INFO("Écrit dans console + fichier (truncate)");

			Logger::clearSinks();
			Logger::addSink(std::cout);
			Q_WARNING("Après clearSinks(): seule la console est réinstallée");
		}

		if (RUN_ASSERT) {
#ifndef NDEBUG
			Q_ASSERT(false, "SelfTest: assert volontaire (va quitter)");
#else
			Q_INFO("NDEBUG actif: Q_ASSERT désactivé (aucun effet).");
#endif
		}

		Logger::setMinLevel(oldMin);
		Logger::enableColors(oldColors);
		Logger::setAbortOnFatal(oldAbort);
		Q_INFO("Logger tests terminé.");*/
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
		//m_NodeEditor.reset();
		//m_AnimationEditorPanel.reset();
		//m_HeightMapEditor.reset();
	}

	void Editor::OnUpdate(double dt)
	{
		Input::Update();

		m_ContentBrowserPanel->Update();
		m_EditorCamera->Update();
		m_SceneManager->Update(dt);
		m_EditorViewport->Update(*m_EditorCamera);
		m_Viewport->Update(m_SceneManager->GetActiveScene(), dt);
		//m_AnimationEditorPanel->Update(dt);
		//m_HeightMapEditor->Update();
		//m_UserInterfaceEditor->Update();

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
		m_Viewport->Render(m_SceneManager->GetActiveScene());
		m_EditorViewport->Render(m_SceneManager->GetActiveScene(), *m_EditorCamera);
	}

	static inline ImVec4 ColorU32ToVec4(ImU32 c) {
		return ImGui::ColorConvertU32ToFloat4(c);
	}

	void Editor::DrawStatsWindow()
	{
		static FrameTimeHistory frame_history;

		const ApplicationInfos& infos = Application::Get().GetAppInfos();
		auto& tracker = MemoryTracker::instance();

		if (!ImGui::Begin("Performance & Memory Tracker"))
		{
			ImGui::End(); return;
		}

		DrawFrameStats(infos, frame_history);

		if (ImGui::BeginTable("##memtbl", 3, ImGuiTableFlags_SizingStretchProp))
		{
			ImGui::TableNextRow();
			ImGui::TableSetColumnIndex(0); ImGui::Text("Total Allocated");
			ImGui::TableSetColumnIndex(1); ImGui::Text("Total Freed");
			ImGui::TableSetColumnIndex(2); ImGui::Text("Current Usage");

			ImGui::TableNextRow();
			auto mb = [](double b) { return b / (1024.0 * 1024.0); };
			ImGui::TableSetColumnIndex(0); ImGui::Text("%.2f MB", mb(tracker.GetTotalAllocated()));
			ImGui::TableSetColumnIndex(1); ImGui::Text("%.2f MB", mb(tracker.GetTotalFreed()));
			ImGui::TableSetColumnIndex(2); ImGui::Text("%.2f MB", mb(tracker.GetCurrentUsage()));
			ImGui::EndTable();
		}

		const auto& memHist = tracker.GetHistory();
		if (!memHist.empty())
		{
			float minv = memHist[0], maxv = memHist[0];
			for (float v : memHist) { if (v < minv) minv = v; if (v > maxv) maxv = v; }
			float range = ImMax(1.0f, (maxv - minv) * 1.1f);
			float smin = maxv - range;
			float smax = maxv + range * 0.05f;

			ImGui::PlotLines("Memory Usage (MB)",
				memHist.data(), (int)memHist.size(), 0,
				nullptr, smin, smax, ImVec2(-1, 90.0f));
		}

		ImGui::End();
	}

	void Editor::DrawFrameStats(const ApplicationInfos& infos, FrameTimeHistory& history)
	{
		static double last_push = 0.0;
		double now = ImGui::GetTime();
		if (now - last_push > 0.25) { history.Push(infos); last_push = now; }

		const double fps = infos.app_fps;
		ImGui::Text("FPS: %.1f | Frame Time: %.2f ms", fps, 1000.0 / ImMax(1.0, fps));
		ImGui::Separator();

		const int size = history.Size();
		if (size < 2) { ImGui::Dummy(ImVec2(ImGui::GetContentRegionAvail().x, 120)); return; }

		ImVec2 plot_size(ImGui::GetContentRegionAvail().x, 120.0f);
		ImDrawList* dl = ImGui::GetWindowDrawList();
		const ImVec2 pos = ImGui::GetCursorScreenPos();
		const ImVec2 pos_max = pos + plot_size;

		static const ImU32 kColors[] = {
			IM_COL32(200,100,255,255), // begin
			IM_COL32(150,230,120,255), // update
			IM_COL32(80,170,255,255), // render
			IM_COL32(255,210,120,255), // end
			IM_COL32(255,100,100,255), // event
			IM_COL32(100,210,180,255), // asset
			IM_COL32(255,110,220,255)  // imgui
		};
		static const char* kLabels[] = { "Begin","Update","Render","End","Event","Asset","ImGui" };
		static bool visible[IM_ARRAYSIZE(kLabels)] = { true,true,true,true,true,true,true };

		float* curves[] = {
			history.begin, history.update, history.render, history.end,
			history.event, history.asset, history.imgui
		};
		const int n_curves = IM_ARRAYSIZE(kLabels);

		constexpr int RECENT_COUNT = 4;
		float max_recent = 0.0f;
		const int lookback = ImMin(size, RECENT_COUNT);
		const int start = history.filled ? history.index : 0;
		for (int i = 0; i < lookback; ++i)
		{
			const int idx = (start + size - lookback + i) % FRAME_HISTORY_SIZE;
			float sum = 0.f;
			for (int c = 0; c < n_curves; ++c) if (visible[c]) sum += curves[c][idx];
			if (sum > max_recent) max_recent = sum;
		}
		max_recent = ImMax(max_recent, 1.5f);
		float max_v = max_recent / 0.95f;
		max_v = ImClamp(max_v, 3.0f, 50.0f);

		dl->AddRectFilled(pos, pos_max, IM_COL32(32, 32, 36, 255), 6.0f);
		dl->AddRect(pos, pos_max, IM_COL32(80, 80, 90, 180), 6.0f);
		ImGui::PushClipRect(pos, pos_max, true);

		const float guides[] = { 8.0f, 16.6667f, 33.3333f };
		for (float g : guides)
		{
			float y = pos.y + plot_size.y - (g / max_v) * plot_size.y;
			ImU32 col = (fabsf(g - 16.6667f) < 0.01f || fabsf(g - 33.3333f) < 0.01f)
				? IM_COL32(200, 200, 255, 80) : IM_COL32(200, 200, 200, 40);
			dl->AddLine(ImVec2(pos.x, y), ImVec2(pos_max.x, y), col, 1.0f);
			dl->AddText(ImVec2(pos.x + 4, y - ImGui::GetTextLineHeight() * 0.5f),
				IM_COL32(220, 220, 220, 160),
				(std::to_string((int)g) + " ms").c_str());
		}

		static std::vector<ImVec2> poly;
		poly.reserve(FRAME_HISTORY_SIZE);

		const float step = plot_size.x / float(size - 1);

		for (int c = 0; c < n_curves; ++c)
		{
			if (!visible[c]) continue;

			poly.clear();
			poly.resize(size);
			for (int i = 0; i < size; ++i)
			{
				const int idx = (start + i) % FRAME_HISTORY_SIZE;
				const float x = pos.x + i * step;
				const float y = pos.y + plot_size.y - (curves[c][idx] / max_v) * plot_size.y;
				poly[i] = ImVec2(x, y);
			}
			dl->AddPolyline(poly.data(), size, kColors[c], false, 2.0f);
		}

		ImGui::PopClipRect();

		ImGui::SetCursorScreenPos(pos);
		ImGui::InvisibleButton("##frame_plot", plot_size, ImGuiButtonFlags_MouseButtonLeft);

		const bool hovered = ImGui::IsItemHovered();
		if (hovered)
		{
			const ImVec2 m = ImGui::GetIO().MousePos;
			float t = (plot_size.x > 0.f) ? (m.x - pos.x) / plot_size.x : 0.f;
			t = ImClamp(t, 0.f, 1.f);
			int i = int(t * (size - 1) + 0.5f);
			const int idx = (start + i) % FRAME_HISTORY_SIZE;

			dl->AddLine(ImVec2(pos.x + i * step, pos.y),
				ImVec2(pos.x + i * step, pos.y + plot_size.y),
				IM_COL32(255, 255, 255, 60), 1.0f);

			float total = 0.f;
			for (int c = 0; c < n_curves; ++c) if (visible[c]) total += curves[c][idx];

			ImGui::BeginTooltip();
			ImGui::Text("Frame #%d", i);
			ImGui::Separator();
			ImGui::Text("Total: %.2f ms (%.0f FPS)", total, total > 0.f ? 1000.f / total : 0.f);
			for (int c = 0; c < n_curves; ++c)
			{
				if (!visible[c]) continue;
				float v = curves[c][idx];
				float pct = (total > 0.f) ? (v / total * 100.f) : 0.f;
				ImGui::ColorButton(("##c" + std::to_string(c)).c_str(), ColorU32ToVec4(kColors[c]),
					ImGuiColorEditFlags_NoTooltip | ImGuiColorEditFlags_NoDragDrop, ImVec2(10, 10));
				ImGui::SameLine();
				ImGui::Text("%-6s  %6.2f ms  (%5.1f%%)", kLabels[c], v, pct);
			}
			ImGui::EndTooltip();
		}

		ImGuiStyle& style = ImGui::GetStyle();
		ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(style.ItemSpacing.x, 0.0f));

		if (ImGui::BeginTable("##legend", n_curves, ImGuiTableFlags_SizingStretchSame))
		{
			for (int c = 0; c < n_curves; ++c)
			{
				ImGui::TableNextColumn();
				ImGui::PushID(c);
				ImGui::Checkbox("##v", &visible[c]); ImGui::SameLine(0, 4);
				ImGui::ColorButton("##clr", ColorU32ToVec4(kColors[c]),
					ImGuiColorEditFlags_NoTooltip | ImGuiColorEditFlags_NoDragDrop, ImVec2(10, 10));
				ImGui::SameLine(0, 4);
				ImGui::TextUnformatted(kLabels[c]);
				ImGui::PopID();
			}
			ImGui::EndTable();
		}

		ImGui::PopStyleVar();
		ImGui::Separator();
	}

	void Editor::OnGuiRender()
	{
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

		//m_Viewport->OnImGuiRender(m_SceneManager->GetActiveScene());
		m_Viewport->OnImGuiRender(m_SceneManager->GetActiveScene());
		m_EditorViewport->OnImGuiRender(*m_EditorCamera, *m_SceneManager, *m_SceneHierarchy);
		m_EntityPropertiePanel->OnImGuiRender(m_SceneManager->GetActiveScene(), *m_SceneHierarchy);
		m_SceneHierarchy->OnImGuiRender(m_SceneManager->GetActiveScene());
		m_ContentBrowserPanel->OnImGuiRender();
		//m_AnimationEditorPanel->OnImGuiRender();
		//m_NodeEditor->OnImGuiRender();
		//m_HeightMapEditor->OnImGuiRender();
		//m_UserInterfaceEditor->OnImGuiRender();

		/*try {
			m_UserInterfaceEditor->OnImGuiRender("UI Editor");
		}
		catch (const std::system_error& e) {
			OutputDebugStringA(("std::system_error: " + std::string(e.what()) + "\n").c_str());
			// mets aussi un __debugbreak() ici
		}*/

		OptionMenu();

#if QE_PROFILE_APP_TIMERS
		DrawStatsWindow();
#endif

		ImGui::End();
	}

	void Editor::OnEvent(Event& e)
	{
		m_EditorCamera->OnEvent(e);

		m_Viewport->OnEvent(e);

		EventDispatcher dispatcher(e);
		dispatcher.Dispatch<MouseButtonPressedEvent>(std::bind(&Editor::OnMouseButtonPressed, this, std::placeholders::_1));
	}

	bool Editor::OnMouseButtonPressed(MouseButtonPressedEvent& e)
	{
		if (ImGuizmo::IsOver())
			return false;

		if (!m_EditorViewport->IsViewportHovered())
			return false;

		//if (e.GetMouseButton() == Mouse::Button0)
		//{
			//m_SceneHierarchy->m_SelectedEntity = m_EditorViewport->GetHoveredEntity();
			//return true;
		//}

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
