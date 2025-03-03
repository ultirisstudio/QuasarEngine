#pragma once

#include <QuasarEngine/Core/GlfwApplication.h>
#include <QuasarEngine/GlfwEditor/GlfwEditorLayer.h>

class GlfwQuasarEditor : public GlfwApplication
{
public:
	GlfwQuasarEditor(const ApplicationSpecification& spec) : GlfwApplication(spec)
	{
		PushLayer(new GlfwEditorLayer());
	}

	~GlfwQuasarEditor()
	{

	}

	void Run() override
	{
		while (true)
		{

		}
	}

	void Close() override
	{

	}
};