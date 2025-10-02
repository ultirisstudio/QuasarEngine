#include "qepch.h"

#include <chrono>

#include "Application.h"
#include "QuasarEngine/Renderer/Renderer.h"
#include "QuasarEngine/Core/Logger.h"

#ifndef QE_PROFILE_APP_TIMERS
#if !defined(NDEBUG)
#define QE_PROFILE_APP_TIMERS 1
#else
#define QE_PROFILE_APP_TIMERS 1
#endif
#endif

extern "C" {
	__declspec(dllexport) unsigned long NvOptimusEnablement = 0x00000001;
	__declspec(dllexport) int AmdPowerXpressRequestHighPerformance = 1;
}

namespace QuasarEngine
{
	namespace
	{
		using clock = std::chrono::steady_clock;
		using dsec = std::chrono::duration<double>;
		using dmsec = std::chrono::duration<double, std::milli>;

		inline void sleep_until_precise(clock::time_point tp) {
			std::this_thread::sleep_until(tp);
		}
	}

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

        auto lastFrameBegin = clock::now();
        auto lastImGuiTick = lastFrameBegin;

        const bool capFps = (m_Specification.MaxFPS > 0);
        const double frameMs = capFps ? (1000.0 / m_Specification.MaxFPS) : 0.0;

        const bool capImGui = (m_Specification.ImGuiMaxFPS > 0);
        const double imguiMs = capImGui ? (1000.0 / m_Specification.ImGuiMaxFPS) : 0.0;

        while (m_Running)
        {
            const auto frameBegin = clock::now();
            const double dt = std::chrono::duration<double>(frameBegin - lastFrameBegin).count();
            lastFrameBegin = frameBegin;

            deltaTime = static_cast<float>(std::min(dt, 0.1));

#if QE_PROFILE_APP_TIMERS
            ApplicationInfos nextInfos{};
#endif

            for (Layer* layer : m_LayerManager)
            {
#if QE_PROFILE_APP_TIMERS
                auto t0 = clock::now();
#endif
                layer->OnUpdate(deltaTime);
#if QE_PROFILE_APP_TIMERS
                auto t1 = clock::now();
                nextInfos.update_latency += std::chrono::duration<double, std::milli>(t1 - t0).count();
#endif

#if QE_PROFILE_APP_TIMERS
                auto t2 = clock::now();
#endif
                layer->OnRender();
#if QE_PROFILE_APP_TIMERS
                auto t3 = clock::now();
                nextInfos.render_latency += std::chrono::duration<double, std::milli>(t3 - t2).count();
#endif
            }

#if QE_PROFILE_APP_TIMERS
            auto tb0 = clock::now();
#endif
            m_Window->BeginFrame();
#if QE_PROFILE_APP_TIMERS
            auto tb1 = clock::now();
            nextInfos.begin_latency = std::chrono::duration<double, std::milli>(tb1 - tb0).count();
#endif

            const bool doImGui = m_ImGuiLayer && (!capImGui || dmsec(clock::now() - lastImGuiTick).count() >= imguiMs);

            if (m_ImGuiLayer && doImGui)
            {
#if QE_PROFILE_APP_TIMERS
                auto ti0 = clock::now();
#endif
                m_ImGuiLayer->Begin();
                for (Layer* layer : m_LayerManager)
                    layer->OnGuiRender();
                m_ImGuiLayer->End();
                lastImGuiTick = clock::now();
#if QE_PROFILE_APP_TIMERS
                auto ti1 = clock::now();
                nextInfos.imgui_latency = std::chrono::duration<double, std::milli>(ti1 - ti0).count();
#endif
            }

#if QE_PROFILE_APP_TIMERS
            auto te0 = clock::now();
#endif
            m_Window->EndFrame();
#if QE_PROFILE_APP_TIMERS
            auto te1 = clock::now();
            nextInfos.end_latency = std::chrono::duration<double, std::milli>(te1 - te0).count();
#endif

#if QE_PROFILE_APP_TIMERS
            auto tev0 = clock::now();
#endif

            switch (m_Specification.EventMode)
            {
            case ApplicationSpecification::EventPumpMode::Poll:
                m_Window->PollEvents();
                break;
            case ApplicationSpecification::EventPumpMode::Wait:
                if (m_Window->WaitEventsTimeout(m_Specification.EventWaitTimeoutSec) == false)
                    m_Window->PollEvents();
                break;
            case ApplicationSpecification::EventPumpMode::Adaptive:
            default:
            {
                if (capFps) {
                    if (m_Window->WaitEventsTimeout(m_Specification.EventWaitTimeoutSec) == false)
                        m_Window->PollEvents();
                }
                else {
                    m_Window->PollEvents();
                }
                break;
            }
            }

#if QE_PROFILE_APP_TIMERS
            auto tev1 = clock::now();
            nextInfos.event_latency = std::chrono::duration<double, std::milli>(tev1 - tev0).count();
#endif

#if QE_PROFILE_APP_TIMERS
            auto ta0 = clock::now();
#endif
            if (Renderer::m_SceneData.m_AssetManager)
                Renderer::m_SceneData.m_AssetManager->Update();
#if QE_PROFILE_APP_TIMERS
            auto ta1 = clock::now();
            nextInfos.asset_latency = std::chrono::duration<double, std::milli>(ta1 - ta0).count();
#endif

            const auto frameEnd = clock::now();
            const double frameTimeMs = dmsec(frameEnd - frameBegin).count();

            if (m_Specification.MinimizedSleep && m_Minimized) {
                std::this_thread::sleep_for(std::chrono::milliseconds(66));
            }
            else if (capFps) {
                const auto targetEnd = frameBegin + std::chrono::milliseconds((long long)(frameMs));
                if (targetEnd > clock::now())
                    sleep_until_precise(targetEnd);
            }

            CalculPerformance(frameTimeMs);

#if QE_PROFILE_APP_TIMERS
            const double fps = m_appInfos.app_fps;
            const double avg = m_appInfos.app_latency;
            m_appInfos = nextInfos;
            m_appInfos.app_fps = fps;
            m_appInfos.app_latency = avg;
#endif
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
		return false;
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
