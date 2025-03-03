#include "Application.h"

/*#include <Windows.h>

extern "C" {
	_declspec(dllexport) DWORD NvOptimusEnablement = 1;
	_declspec(dllexport) int AmdPowerXpressRequestHighPerformance = 1;
}*/

Application* Application::s_Instance = nullptr;
	
Application::Application(const ApplicationSpecification& specification) : m_Specification(specification)
{
	m_Window = EngineFactory::Instance().CreateWindow(specification.WindowAPI);
	m_Renderer = EngineFactory::Instance().CreateRenderer(specification.RendererAPI);

	m_Window->Initialize();
	m_Renderer->Initialize();
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
	
}