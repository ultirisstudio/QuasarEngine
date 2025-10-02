#pragma once

#include "QuasarEngine/Core/Core.h"
#include "QuasarEngine/Events/Event.h"
#include "QuasarEngine/Events/ApplicationEvent.h"
#include "QuasarEngine/Core/Window.h"
#include "QuasarEngine/Core/LayerManager.h"
#include "QuasarEngine/Core/Layer.h"
#include "QuasarEngine/ImGui/ImGuiLayer.h"
#include "QuasarEngine/Renderer/Renderer.h"
#include <QuasarEngine/Events/MouseEvent.h>

int main(int argc, char** argv);

namespace QuasarEngine
{
	struct ApplicationCommandLineArgs
	{
		int Count = 0;
		char** Args = nullptr;

		const char* operator[](int index) const
		{
			if (index >= Count)
				return nullptr;

			return Args[index];
		}
	};

	struct ApplicationSpecification
	{
		std::string Name = "Quasar Application";
		std::string WorkingDirectory;
		bool EnableImGui = true;
		ApplicationCommandLineArgs CommandLineArgs;

		int    MaxFPS = 0;
		int    ImGuiMaxFPS = 0;
		bool   MinimizedSleep = true;

		enum class EventPumpMode { Poll, Wait, Adaptive };
		EventPumpMode EventMode = EventPumpMode::Adaptive;
		double EventWaitTimeoutSec = 0.01;
	};

	struct ApplicationInfos
	{
		int app_nb_frame = 0;
		double app_fps = 0.0;
		double app_latency = 0.0;

		double begin_latency = 0, update_latency = 0, render_latency = 0, end_latency = 0, event_latency = 0, asset_latency = 0, imgui_latency = 0;
	};

	class Application
	{
	public:
		Application(const ApplicationSpecification& specification);
		virtual ~Application();

		void Run();

		void PushLayer(Layer* layer);
		void PushOverlay(Layer* overlay);

		void OnEvent(Event& e);

		void MaximizeWindow(bool value);

		inline Window& GetWindow() { return *m_Window; }
		inline static Application& Get() { return *s_Instance; }

		const ApplicationSpecification& GetSpecification() const { return m_Specification; }

		const ApplicationInfos& GetAppInfos() const { return m_appInfos; }

		void Close();
	private:
		bool OnWindowClose(WindowCloseEvent& e);
		bool OnWindowResize(WindowResizeEvent& e);
		bool OnMouseMove(MouseMovedEvent& e);

		void CalculPerformance(double frameTimeMs);
	private:
		ApplicationSpecification m_Specification;

		std::atomic<bool> m_Running{ true };
		std::atomic<bool> m_Running2{ true };
		bool m_Minimized = false;

		float deltaTime = 0;

		bool m_can_calcul_latency = false;

		ApplicationInfos m_appInfos;

		static Application* s_Instance;

		std::unique_ptr<Window> m_Window;
		std::unique_ptr<ImGuiLayer> m_ImGuiLayer;
		LayerManager m_LayerManager;
	};

	Application* CreateApplication(ApplicationCommandLineArgs args);
}
