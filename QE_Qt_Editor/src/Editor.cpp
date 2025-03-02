#include <QuasarEngine/EngineFactory.h>

#include <QuasarEngine/OpenGLRenderer.h>
#include <QuasarEngine/DirectXRenderer.h>
#include <QuasarEngine/VulkanRenderer.h>

#include <QuasarEngine/ImGuiGUI.h>
#include <QuasarEngine/QtGUI.h>

#include <QuasarEngine/GLFWWindow.h>
#include <QuasarEngine/QtWindow.h>

#include <QtWidgets/QApplication>
#include <QtWidgets/QWidget>

int main(int argc, char* argv[])
{
	OpenGLRenderer::Register();
	//DirectXRenderer::Register();
	//VulkanRenderer::Register();

	//ImGuiGUI::Register();
	QtGUI::Register();

	//GLFWWindow::Register();
	QtWindow::Register(argc, argv);

	std::unique_ptr<IRenderer> renderer = EngineFactory::Instance().CreateRenderer(RendererAPI::OpenGL);
	std::unique_ptr<IGUI> gui = EngineFactory::Instance().CreateGUI(GUIAPI::Qt);
	std::unique_ptr<IWindow> window = EngineFactory::Instance().CreateWindow(WindowAPI::Qt);

	renderer->Initialize();
	gui->Initialize();
	window->Initialize();

	if (dynamic_cast<GLFWWindow*>(window.get())) {
		while (!static_cast<GLFWWindow*>(window.get())->ShouldClose()) {
			window->PollEvents();
			renderer->Render();
			gui->Render();
			window->SwapBuffers();
		}
	}
	else {
		//window->Run();

		/*QApplication app(argc, argv);
		QWidget widget;
		widget.resize(800, 600);
		widget.setWindowTitle("Hello World");
		widget.show();
		app.exec();*/
	}

	window->Close();

	return 0;
}