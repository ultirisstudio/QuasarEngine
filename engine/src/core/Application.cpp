#include <quasar/core/Application.h>
#include <quasar/core/Log.h>

#include <utility>

namespace Quasar
{
    namespace
    {
        Application* s_Instance = nullptr;
    }

    Application::Application(Scope<Window> window) : m_Window(std::move(window))
    {
        s_Instance = this;
    }

    Application::~Application()
    {
        s_Instance = nullptr;
    }

    Scope<Application> Application::Create(const ApplicationSpecification& specification)
    {
        if (s_Instance != nullptr)
        {
            QUASAR_ENGINE_ERROR("Cannot create the application '{}': an application already exists",
                                specification.Window.Title);
            return nullptr;
        }

        Scope<Window> window = Window::Create(specification.Window);
        if (window == nullptr)
        {
            QUASAR_ENGINE_ERROR("Cannot create the application '{}': its window cannot be created",
                                specification.Window.Title);
            return nullptr;
        }

        window->SetVSync(false);

        return Scope<Application>(new Application(std::move(window)));
    }

    Application& Application::Get()
    {
        QUASAR_ENGINE_ASSERT(s_Instance != nullptr, "No application exists");
        return *s_Instance;
    }

    void Application::Run()
    {
        m_Running = true;
        m_FrameTimer.Reset();
        m_LastFrameTime = 0.0;

        while (m_Running && !m_Window->ShouldClose())
        {
            const double time = m_FrameTimer.ElapsedSeconds();
            const Timestep timestep(static_cast<float>(time - m_LastFrameTime));
            m_LastFrameTime = time;

            for (Scope<Layer>& layer : m_LayerStack)
            {
                layer->OnUpdate(timestep);
            }

            for (Scope<Layer>& layer : m_LayerStack)
            {
                layer->OnRender();
            }

            m_Window->Update();
        }

        m_Running = false;
    }

    void Application::Close()
    {
        m_Running = false;
    }

    Layer& Application::PushLayer(Scope<Layer> layer)
    {
        return m_LayerStack.PushLayer(std::move(layer));
    }

    Layer& Application::PushOverlay(Scope<Layer> overlay)
    {
        return m_LayerStack.PushOverlay(std::move(overlay));
    }

    Scope<Layer> Application::PopLayer(Layer& layer)
    {
        return m_LayerStack.PopLayer(layer);
    }

    Scope<Layer> Application::PopOverlay(Layer& overlay)
    {
        return m_LayerStack.PopOverlay(overlay);
    }
}
