#pragma once

#include <QuasarEngine/Core/Core.h>
#include <QuasarEngine/Core/LayerManager.h>
#include <QuasarEngine/Core/Layer.h>

#include <QuasarEngine/EngineFactory.h>

#include <QuasarEngine/Core/Application.h>

class QWidget;

class QtApplication : public Application
{
public:
	struct QtApplicationData
	{
		QWidget* widget;
	};

public:
	QtApplication(const ApplicationSpecification& specification);
	~QtApplication();

	void Run() override;

	const QtApplicationData& getApplicationData() { return m_ApplicationData; }

protected:
	QtApplicationData m_ApplicationData;
};

Application* CreateApplication(ApplicationCommandLineArgs args);
