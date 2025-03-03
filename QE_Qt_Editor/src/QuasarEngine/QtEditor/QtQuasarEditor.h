#pragma once

#include <QuasarEngine/Core/QtApplication.h>
#include <QuasarEngine/QtEditor/QtEditorLayer.h>

#include <QtWidgets/QApplication>

class QtQuasarEditor : public QtApplication
{
public:
	QtQuasarEditor(ApplicationSpecification spec) : QtApplication(spec)
	{
		QApplication app(spec.CommandLineArgs.Count, spec.CommandLineArgs.Args);

		m_ApplicationData.widget = new QtEditorLayer();
		m_ApplicationData.widget->resize(800, 1200);
		m_ApplicationData.widget->setWindowTitle("Hello World");
		m_ApplicationData.widget->show();

		PushLayer(new QtEditorLayer());

		app.exec();
	}

	~QtQuasarEditor()
	{

	}

	void Run() override
	{

	}

	void Close() override
	{

	}
};