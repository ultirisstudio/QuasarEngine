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

	m_Window = EngineFactory::Instance().CreateWindow(specification.WindowAPI);
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
	while (m_Running)
	{
		if (!m_Minimized)
		{
			for (Layer* layer : m_LayerManager)
			{
				layer->OnUpdate(0.0);
			}
		}

		if (m_Specification.EnableImGui)
		{
			
		}
	}
}