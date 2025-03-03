#include <QuasarEngine/EngineFactory.h>

#include <QuasarEngine/Renderer/OpenGLRenderer.h>
#include <QuasarEngine/Renderer/VulkanRenderer.h>
#include <QuasarEngine/Renderer/DirectXRenderer.h>

#include <QuasarEngine/Window/GLFWWindow.h>
#include <QuasarEngine/Window/QtWindow.h>

#include <QuasarEngine/Core/EntryPoint.h>

#include <QuasarEngine/GlfwEditor/GlfwQuasarEditor.h>
#include <QuasarEngine/QtEditor/QtQuasarEditor.h>

Application* CreateApplication(ApplicationCommandLineArgs args)
{
	ApplicationSpecification spec;
	spec.Name = "Quasar Editor";
	spec.CommandLineArgs = args;

	GLFWWindow::Register();
	QtWindow::Register();

	OpenGLRenderer::Register();
	VulkanRenderer::Register();
	DirectXRenderer::Register();

	spec.WindowAPI = WindowAPI::GLFW;

	for (int i = 0; i < args.Count; i++)
	{
		std::string arg(args[i]);

		if (arg == "--opengl")
		{
			spec.RendererAPI = RendererAPI::OpenGL;
		}
		else if (arg == "--vulkan")
		{
			spec.RendererAPI = RendererAPI::Vulkan;
		}
		else if (arg == "--directx")
		{
			spec.RendererAPI = RendererAPI::DirectX;
		}
		else if (arg == "--qt")
		{
			spec.WindowAPI = WindowAPI::Qt;
		}
		else if (arg == "--glfw")
		{
			spec.WindowAPI = WindowAPI::GLFW;
		}
	}

	if (spec.WindowAPI == WindowAPI::GLFW)
	{
		return new GlfwQuasarEditor(spec);
	}

	if (spec.WindowAPI == WindowAPI::Qt)
	{
		return new QtQuasarEditor(spec);
	}

	return nullptr;
}