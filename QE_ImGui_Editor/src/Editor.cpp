#include <QuasarEngine/EngineFactory.h>

#include <QuasarEngine/OpenGLRenderer.h>
#include <QuasarEngine/DirectXRenderer.h>
#include <QuasarEngine/VulkanRenderer.h>

#include <QuasarEngine/ImGuiGUI.h>
#include <QuasarEngine/QtGUI.h>

#include <QuasarEngine/GLFWWindow.h>
#include <QuasarEngine/QtWindow.h>

int main(int argc, char* argv[])
{
	OpenGLRenderer::Register();
	DirectXRenderer::Register();
	VulkanRenderer::Register();

	ImGuiGUI::Register();
	QtGUI::Register();

	GLFWWindow::Register();
	QtWindow::Register(argc, argv);

	std::unique_ptr<IRenderer> renderer = EngineFactory::Instance().CreateRenderer(RendererAPI::Vulkan);
	std::unique_ptr<IGUI> gui = EngineFactory::Instance().CreateGUI(GUIAPI::Qt);
	std::unique_ptr<IWindow> window = EngineFactory::Instance().CreateWindow(WindowAPI::GLFW);

	renderer->Initialize();
	gui->Initialize();
	window->Initialize();

	for (int i = 0; i < 3; i++)
	{
		window->PollEvents();

		renderer->Render();
		gui->Render();

		//window->Show();
	}

	renderer->Shutdown();
	gui->Shutdown();

	window->Close();

	return 0;
}