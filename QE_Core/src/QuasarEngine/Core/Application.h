#pragma once

#include "QuasarEngine/Core/Core.h"
#include "QuasarEngine/Core/LayerManager.h"
#include "QuasarEngine/Core/Layer.h"

#include "QuasarEngine/EngineFactory.h"

int main(int argc, char** argv);

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

	WindowAPI windowAPI = WindowAPI::GLFW;
	RendererAPI rendererAPI = RendererAPI::OpenGL;

	ApplicationCommandLineArgs CommandLineArgs;
};

class Application
{
public:
	Application(const ApplicationSpecification& specification);
	virtual ~Application();

	void Run();

	void PushLayer(Layer* layer);
	void PushOverlay(Layer* overlay);

	inline IWindow& GetWindow() { return *m_Window; }
	inline static Application& Get() { return *s_Instance; }

	const ApplicationSpecification& GetSpecification() const { return m_Specification; }

	void Close();

private:
	ApplicationSpecification m_Specification;

	bool m_Running = true;
	bool m_Minimized = false;

	std::unique_ptr<IWindow> m_Window;
	std::unique_ptr<IRenderer> m_Renderer;

	LayerManager m_LayerManager;

	static Application* s_Instance;
};

Application* CreateApplication(ApplicationCommandLineArgs args);
