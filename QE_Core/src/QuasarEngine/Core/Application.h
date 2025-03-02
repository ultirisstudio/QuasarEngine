#pragma once

#include "QuasarEngine/Core/Core.h"
#include "QuasarEngine/Core/LayerManager.h"
#include "QuasarEngine/Core/Layer.h"

#include "QuasarEngine/EngineFactory.h"
#include "QuasarEngine/Window/IWindow.h"

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
	bool EnableImGui = true;

	WindowAPI WindowAPI = WindowAPI::GLFW;

	ApplicationCommandLineArgs CommandLineArgs;
};

struct ApplicationInfos
{
	int app_nb_frame = 0;
	int app_fps = 0;
	int app_latency = 0;
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

	const ApplicationInfos& GetAppInfos() const { return m_appInfos; }

	void Close();

private:
	ApplicationSpecification m_Specification;

	bool m_Running = true;
	bool m_Minimized = false;

	ApplicationInfos m_appInfos;

	std::unique_ptr<IWindow> m_Window;
	LayerManager m_LayerManager;

	static Application* s_Instance;
};

Application* CreateApplication(ApplicationCommandLineArgs args);
