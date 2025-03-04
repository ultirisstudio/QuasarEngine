#pragma once

#include <QuasarEngine/Core/Core.h>
#include <QuasarEngine/Core/LayerManager.h>
#include <QuasarEngine/Core/Layer.h>

#include <QuasarEngine/EngineFactory.h>

#include <QuasarEngine/Core/Application.h>

#include <QuasarEngine/Window/GLFWWindow.h>

class GlfwApplication : public Application
{
public:
	struct GlfwApplicationData
	{
		
	};

public:
	GlfwApplication(const ApplicationSpecification& specification);
	~GlfwApplication();

	void Run() override;

	const GlfwApplicationData& getApplicationData() { return m_ApplicationData; }

protected:
	GlfwApplicationData m_ApplicationData;

	GlfwWindow* m_GlfwWindow;
};

Application* CreateApplication(ApplicationCommandLineArgs args);
