#include "qepch.h"

#include <chrono>

#include "Application.h"
#include "QuasarEngine/Renderer/Renderer.h"
#include "QuasarEngine/Core/Logger.h"

namespace QuasarEngine
{
	Application* Application::s_Instance = nullptr;
	
	Application::Application(const ApplicationSpecification& specification) : m_Specification(specification)
	{
		Q_ASSERT(s_Instance == nullptr, "Only one Application instance allowed");
		s_Instance = this;

		m_Window = std::make_unique<Window>();
		m_Window->SetEventCallback(std::bind(&Application::OnEvent, this, std::placeholders::_1));
		m_Window->SetVSync(false);

		if (m_Specification.EnableImGui)
		{
			m_ImGuiLayer = ImGuiLayer::Create();
			PushOverlay(m_ImGuiLayer.get());
		}

		Renderer::Init();
	}

	Application::~Application()
	{
		if (m_Specification.EnableImGui)
		{
			m_ImGuiLayer.reset();
		}

		Renderer::Shutdown();

		s_Instance = nullptr;
	}

	void Application::OnEvent(Event& e)
	{
		EventDispatcher dispatcher(e);
		dispatcher.Dispatch<WindowCloseEvent>(std::bind(&Application::OnWindowClose, this, std::placeholders::_1));
		dispatcher.Dispatch<WindowResizeEvent>(std::bind(&Application::OnWindowResize, this, std::placeholders::_1));
		dispatcher.Dispatch<MouseMovedEvent>(std::bind(&Application::OnMouseMove, this, std::placeholders::_1));

		//for (Layer* layer : m_LayerManager)
		//	layer->OnEvent(e);

		for (auto it = m_LayerManager.rbegin(); it != m_LayerManager.rend(); ++it)
		{
			(*it)->OnEvent(e);
			if (e.Handled) break;
		}
	}

	void Application::MaximizeWindow(bool value)
	{
		m_Window->SetMaximized(value);
	}

	void Application::PushLayer(Layer* layer)
	{
		m_LayerManager.PushLayer(layer);
		layer->OnAttach();
	}

	void Application::PushOverlay(Layer* layer)
	{
		m_LayerManager.PushOverlay(layer);
		layer->OnAttach();
	}

	void Application::Close()
	{
		m_Running = false;
	}

	void Application::Run()
	{
		using clock = std::chrono::steady_clock;
		auto lastTime = clock::now();

		while (m_Running)
		{
			auto frameStart = clock::now();

			auto now = clock::now();
			deltaTime = std::chrono::duration<float>(now - lastTime).count();
			lastTime = now;
			if (deltaTime > 0.1f) deltaTime = 0.1f;

			ApplicationInfos nextInfos{};

			for (Layer* layer : m_LayerManager)
			{
				auto t0 = clock::now();
				layer->OnUpdate(deltaTime);
				auto t1 = clock::now();
				nextInfos.update_latency += std::chrono::duration<double, std::milli>(t1 - t0).count();

				auto t2 = clock::now();
				layer->OnRender();
				auto t3 = clock::now();
				nextInfos.render_latency += std::chrono::duration<double, std::milli>(t3 - t2).count();
			}

			auto tb0 = clock::now();
			m_Window->BeginFrame();
			auto tb1 = clock::now();
			nextInfos.begin_latency = std::chrono::duration<double, std::milli>(tb1 - tb0).count();

			if (m_ImGuiLayer)
			{
				auto ti0 = clock::now();
				m_ImGuiLayer->Begin();
				for (Layer* layer : m_LayerManager)
					layer->OnGuiRender();
				m_ImGuiLayer->End();
				auto ti1 = clock::now();
				nextInfos.imgui_latency = std::chrono::duration<double, std::milli>(ti1 - ti0).count();
			}

			auto te0 = clock::now();
			m_Window->EndFrame();
			auto te1 = clock::now();
			nextInfos.end_latency = std::chrono::duration<double, std::milli>(te1 - te0).count();

			auto tev0 = clock::now();
			m_Window->PollEvents();
			auto tev1 = clock::now();
			nextInfos.event_latency = std::chrono::duration<double, std::milli>(tev1 - tev0).count();

			auto ta0 = clock::now();
			if (Renderer::m_SceneData.m_AssetManager)
				Renderer::m_SceneData.m_AssetManager->Update();
			auto ta1 = clock::now();
			nextInfos.asset_latency = std::chrono::duration<double, std::milli>(ta1 - ta0).count();

			const double frameTimeMs =
				std::chrono::duration<double, std::milli>(clock::now() - frameStart).count();
			CalculPerformance(frameTimeMs);

			const double fps = m_appInfos.app_fps;
			const double avg = m_appInfos.app_latency;
			m_appInfos = nextInfos;
			m_appInfos.app_fps = fps;
			m_appInfos.app_latency = avg;
		}

		for (Layer* layer : m_LayerManager)
			layer->OnDetach();
	}

	void Application::CalculPerformance(double frameTimeMs)
	{
		using clock = std::chrono::steady_clock;
		static auto lastSample = clock::now();
		static double accMs = 0.0;
		static int frames = 0;
		static double emaMs = frameTimeMs;

		++frames; accMs += frameTimeMs; emaMs += 0.1 * (frameTimeMs - emaMs);

		const double elapsedMs = std::chrono::duration<double, std::milli>(clock::now() - lastSample).count();
		if (elapsedMs >= 1000.0) {
			const double avgMs = frames ? (accMs / frames) : 0.0;
			const double fps = elapsedMs ? (frames * 1000.0 / elapsedMs) : 0.0;

			m_appInfos.app_fps = fps;
			m_appInfos.app_latency = avgMs;

			std::ostringstream title;
			title.imbue(std::locale::classic());
			title.setf(std::ios::fixed);
			title.precision(1);
			title << m_Specification.Name << " [" << static_cast<int>(fps + 0.5) << " FPS / " << avgMs << " ms avg | " << emaMs << " ms EMA]";

			m_Window->SetTitle(title.str());

			lastSample = clock::now();
			accMs = 0.0; frames = 0;
		}
	}

	bool Application::OnWindowClose(WindowCloseEvent& e)
	{
		m_Running = false;
		return true;
	}

	bool Application::OnWindowResize(WindowResizeEvent& e)
	{
		if (e.GetWidth() == 0 || e.GetHeight() == 0)
		{
			m_Minimized = true;
			return false;
		}

		m_Minimized = false;

		m_Window->Resize(e.GetWidth(), e.GetHeight());

		return false;
	}

	bool Application::OnMouseMove(MouseMovedEvent& e) {
		m_Window->SetMousePosition(static_cast<float>(e.GetX()), static_cast<float>(e.GetY()));
		return false;
	}
}
