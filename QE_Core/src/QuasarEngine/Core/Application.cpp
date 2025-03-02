#include "Application.h"

/*#include <Windows.h>

extern "C" {
	_declspec(dllexport) DWORD NvOptimusEnablement = 1;
	_declspec(dllexport) int AmdPowerXpressRequestHighPerformance = 1;
}*/

Application* Application::s_Instance = nullptr;
	
Application::Application(const ApplicationSpecification& specification) : m_Specification(specification)
{
	s_Instance = this;

	m_Window = EngineFactory::Instance().CreateWindow(specification.windowAPI);
	m_Renderer = EngineFactory::Instance().CreateRenderer(specification.rendererAPI);

	m_Window->Initialize();
	m_Renderer->Initialize();
}

Application::~Application()
{
		
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
	if (m_Specification.windowAPI == WindowAPI::GLFW)
	{
		while (m_Running)
		{
			if (!m_Minimized)
			{
				for (Layer* layer : m_LayerManager)
				{
					layer->OnUpdate(0.0);
				}
			}

			m_Window->PollEvents();
			m_Renderer->Render();
			m_Window->SwapBuffers();
		}
	}

	if (m_Specification.windowAPI == WindowAPI::Qt)
	{
		m_Window->Run();
	}
}