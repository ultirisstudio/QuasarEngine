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

	WindowAPI WindowAPI = WindowAPI::GLFW;
	RendererAPI RendererAPI = RendererAPI::OpenGL;

	ApplicationCommandLineArgs CommandLineArgs;
};

class Application
{
public:
	Application(const ApplicationSpecification& specification);
	~Application() = default;

	void PushLayer(Layer* layer);
	void PushOverlay(Layer* overlay);

	inline IWindow& GetWindow() { return *m_Window; }
	inline static Application& Get() { return *s_Instance; }

	const ApplicationSpecification& GetSpecification() const { return m_Specification; }

	virtual void Run() = 0;
	virtual void Close() = 0;

protected:
	ApplicationSpecification m_Specification;

	std::unique_ptr<IWindow> m_Window;
	std::unique_ptr<IRenderer> m_Renderer;

	LayerManager m_LayerManager;

	static Application* s_Instance;
};

Application* CreateApplication(ApplicationCommandLineArgs args);
