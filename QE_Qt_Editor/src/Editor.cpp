#include <QuasarEngine/EngineFactory.h>

#include <QuasarEngine/OpenGLRenderer.h>
#include <QuasarEngine/DirectXRenderer.h>
#include <QuasarEngine/VulkanRenderer.h>

#include <QuasarEngine/ImGuiGUI.h>
#include <QuasarEngine/QtGUI.h>

#include <QuasarEngine/GLFWWindow.h>
#include <QuasarEngine/QtWindow.h>

#include <QuasarEngine/Core/EntryPoint.h>

/*int main(int argc, char* argv[])
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

		//QApplication app(argc, argv);
		//QWidget widget;
		//widget.resize(800, 600);
		//widget.setWindowTitle("Hello World");
		//widget.show();
		//app.exec();
	}

	window->Close();

	return 0;
}

OpenGLRenderer::Register();
	//DirectXRenderer::Register();
	//VulkanRenderer::Register();

	//ImGuiGUI::Register();
	QtGUI::Register();

	GLFWWindow::Register();
	//QtWindow::Register(spec.CommandLineArgs.Count, spec.CommandLineArgs.Args);
*/

class Editor : public Layer
{
public:
	Editor()
	{

	}

	~Editor() = default;

	void OnAttach() override
	{

	}

	void OnDetach() override
	{

	}

	void OnUpdate(double dt) override
	{

	}
};

class QuasarEditor : public Application
{
public:
	QuasarEditor(const ApplicationSpecification& spec) : Application(spec)
	{
		PushLayer(new Editor());
	}

	~QuasarEditor()
	{

	}
};

Application* CreateApplication(ApplicationCommandLineArgs args)
{
	ApplicationSpecification spec;
	spec.Name = "Quasar Editor";
	spec.CommandLineArgs = args;

	for (int i = 0; i < args.Count; i++)
	{
		std::string arg(args[i]);

		if (arg == "--opengl")
		{
			OpenGLRenderer::Register();
			spec.rendererAPI = RendererAPI::OpenGL;
		}
		else if (arg == "--vulkan")
		{
			VulkanRenderer::Register();
			spec.rendererAPI = RendererAPI::Vulkan;
		}
		else if (arg == "--directx")
		{
			DirectXRenderer::Register();
			spec.rendererAPI = RendererAPI::DirectX;
		}
		else if (arg == "--qt")
		{
			QtWindow::Register(args.Count, args.Args);
			spec.windowAPI = WindowAPI::Qt;
		}
		else if (arg == "--glfw")
		{
			GLFWWindow::Register();
			spec.windowAPI = WindowAPI::GLFW;
		}
	}

	return new QuasarEditor(spec);
}