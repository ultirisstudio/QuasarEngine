#include "qepch.h"

#include <GLFW/glfw3.h>

#include <chrono>

#include "Application.h"
#include "QuasarEngine/Renderer/Renderer.h"
#include "QuasarEngine/Core/Input.h"

namespace QuasarEngine
{
	Application* Application::s_Instance = nullptr;
	
	Application::Application(const ApplicationSpecification& specification) : m_Specification(specification)
	{
		s_Instance = this;

		m_Window = std::make_unique<Window>();
		m_Window->SetEventCallback(std::bind(&Application::OnEvent, this, std::placeholders::_1));
		m_Window->SetVSync(false);

		m_ImGuiLayer = ImGuiLayer::Create();
		PushOverlay(m_ImGuiLayer.get());

		Renderer::Init();
	}

	Application::~Application()
	{
		Renderer::Shutdown();

		m_ImGuiLayer.reset();
	}

	void Application::OnEvent(Event& e)
	{
		EventDispatcher dispatcher(e);
		dispatcher.Dispatch<WindowCloseEvent>(std::bind(&Application::OnWindowClose, this, std::placeholders::_1));
		dispatcher.Dispatch<WindowResizeEvent>(std::bind(&Application::OnWindowResize, this, std::placeholders::_1));
		dispatcher.Dispatch<MouseMovedEvent>(std::bind(&Application::OnMouseMove, this, std::placeholders::_1));

		for (Layer* layer : m_LayerManager)
			layer->OnEvent(e);
	}

	void Application::MaximizeWindow(bool value)
	{
		if (value)
			glfwMaximizeWindow(static_cast<GLFWwindow*>(m_Window->GetNativeWindow()));
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
		using clock = std::chrono::high_resolution_clock;
		auto lastTime = clock::now();
		perf_last_time = std::chrono::duration<double>(lastTime.time_since_epoch()).count();

		while (m_Running)
		{
			auto frameStart = clock::now();

			auto now = clock::now();
			deltaTime = std::chrono::duration<float>(now - lastTime).count();
			lastTime = now;

			m_appInfos.update_latency = 0.0;
			m_appInfos.render_latency = 0.0;
			if (!m_Minimized)
			{
				for (Layer* layer : m_LayerManager)
				{
					auto t_update0 = clock::now();
					layer->OnUpdate(deltaTime);
					auto t_update1 = clock::now();
					m_appInfos.update_latency += std::chrono::duration<double, std::milli>(t_update1 - t_update0).count();

					auto t_render0 = clock::now();
					layer->OnRender();
					auto t_render1 = clock::now();
					m_appInfos.render_latency += std::chrono::duration<double, std::milli>(t_render1 - t_render0).count();
				}
			}

			auto t0 = clock::now();
			m_Window->BeginFrame();
			auto t1 = clock::now();
			m_appInfos.begin_latency = std::chrono::duration<double, std::milli>(t1 - t0).count();

			auto t2 = clock::now();

			if (m_Specification.EnableImGui)
			{
				m_ImGuiLayer->Begin();
				for (Layer* layer : m_LayerManager)
					layer->OnGuiRender();
				m_ImGuiLayer->End();
			}
			
			auto t3 = clock::now();
			m_appInfos.imgui_latency = std::chrono::duration<double, std::milli>(t3 - t2).count();

			auto t4 = clock::now();
			m_Window->EndFrame();
			auto t5 = clock::now();
			m_appInfos.end_latency = std::chrono::duration<double, std::milli>(t5 - t4).count();

			auto t6 = clock::now();
			m_Window->PollEvents();
			auto t7 = clock::now();
			m_appInfos.event_latency = std::chrono::duration<double, std::milli>(t7 - t6).count();

			auto t8 = clock::now();
			Renderer::m_SceneData.m_AssetManager->Update();
			auto t9 = clock::now();
			m_appInfos.asset_latency = std::chrono::duration<double, std::milli>(t9 - t8).count();

			auto frameEnd = clock::now();
			double frameTimeMs = std::chrono::duration<double, std::milli>(frameEnd - frameStart).count();

			CalculPerformance(frameTimeMs);
		}

		for (Layer* layer : m_LayerManager)
			layer->OnDetach();
	}

	void Application::CalculPerformance(double frameTimeMs)
	{
		double currentTime = std::chrono::duration<double>(std::chrono::high_resolution_clock::now().time_since_epoch()).count();
		m_appInfos.app_nb_frame++;

		static double minFrameTime = std::numeric_limits<double>::max();
		static double maxFrameTime = 0.0;
		minFrameTime = std::min(minFrameTime, frameTimeMs);
		maxFrameTime = std::max(maxFrameTime, frameTimeMs);

		if (currentTime - perf_last_time >= 1.0) {
			m_appInfos.app_fps = m_appInfos.app_nb_frame;
			m_appInfos.app_latency = (m_appInfos.app_nb_frame > 0) ? (1000.0 / double(m_appInfos.app_nb_frame)) : 0.0;

			/*std::cout << std::fixed << std::setprecision(3);
			std::cout << "\n---------------------" << std::endl;
			std::cout << "FPS: " << m_appInfos.app_fps << " | Avg Latency: " << m_appInfos.app_latency << " ms" << std::endl;
			std::cout << "Frame time: avg ~" << frameTimeMs << " ms [min: " << minFrameTime << " ms, max: " << maxFrameTime << " ms]\n";
			std::cout << "begin: " << m_appInfos.begin_latency << " ms" << std::endl;
			std::cout << "update: " << m_appInfos.update_latency << " ms" << std::endl;
			std::cout << "render: " << m_appInfos.render_latency << " ms" << std::endl;
			std::cout << "end: " << m_appInfos.end_latency << " ms" << std::endl;
			std::cout << "event: " << m_appInfos.event_latency << " ms" << std::endl;
			std::cout << "asset: " << m_appInfos.asset_latency << " ms" << std::endl;
			std::cout << "imgui: " << m_appInfos.imgui_latency << " ms" << std::endl;*/

			m_Window->SetTitle(m_Specification.Name + " [" +
				std::to_string(int(m_appInfos.app_fps)) + " FPS / " +
				std::to_string(int(frameTimeMs)) + " ms]");

			m_appInfos.app_nb_frame = 0;
			perf_last_time += 1.0;
			minFrameTime = std::numeric_limits<double>::max();
			maxFrameTime = 0.0;
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
		//RenderCommand::SetViewport(0, 0, e.GetWidth(), e.GetHeight());

		m_Window->Resize(e.GetWidth(), e.GetHeight());

		return false;
	}

	bool Application::OnMouseMove(MouseMovedEvent& e) {
		Window::WindowData& data = *(Window::WindowData*)glfwGetWindowUserPointer(reinterpret_cast<GLFWwindow*>(Application::Get().GetWindow().GetNativeWindow()));
		data.MousePos.x = e.GetX();
		data.MousePos.y = e.GetY();
		return false;
	}
}
