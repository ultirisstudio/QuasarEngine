#include <quasar/core/Application.h>
#include <quasar/core/Log.h>
#include <quasar/events/EventDispatcher.h>
#include <quasar/events/WindowEvent.h>

#include <spdlog/fmt/std.h>

#include <system_error>
#include <utility>

namespace Quasar
{
    namespace
    {
        // The only application, reached from anywhere with Application::Get()
        // NOLINTNEXTLINE(cppcoreguidelines-avoid-non-const-global-variables)
        Application* s_Instance = nullptr;
    }

    Application::Application(ApplicationSpecification specification, Scope<Window> window)
        : m_Specification(std::move(specification)), m_Window(std::move(window))
    {
        s_Instance = this;
        m_Window->SetEventCallback([this](Event& event) { OnEvent(event); });
    }

    Application::~Application()
    {
        m_Window->SetEventCallback({});
        s_Instance = nullptr;
    }

    Scope<Application> Application::Create(const ApplicationSpecification& specification)
    {
        if (s_Instance != nullptr)
        {
            QUASAR_ENGINE_ERROR("Cannot create the application '{}': an application already exists",
                                specification.Name);
            return nullptr;
        }

        if (!specification.WorkingDirectory.empty())
        {
            std::error_code error;
            std::filesystem::current_path(specification.WorkingDirectory, error);
            if (error)
            {
                QUASAR_ENGINE_ERROR("Cannot create the application '{}': cannot use '{}' as working directory: {}",
                                    specification.Name, specification.WorkingDirectory, error.message());
                return nullptr;
            }
        }

        Scope<Window> window = Window::Create(specification.Window);
        if (window == nullptr)
        {
            QUASAR_ENGINE_ERROR("Cannot create the application '{}': its window cannot be created", specification.Name);
            return nullptr;
        }

        QUASAR_ENGINE_INFO("Created the application '{}'", specification.Name);
        return Scope<Application>(new Application(specification, std::move(window)));
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

            const bool canRender = !m_Minimized && m_Window->GetWidth() > 0 && m_Window->GetHeight() > 0;
            if (canRender)
            {
                for (Scope<Layer>& layer : m_LayerStack)
                {
                    layer->OnRender();
                }
            }

            m_Window->Update();
        }

        m_Running = false;
    }

    void Application::OnEvent(Event& event)
    {
        EventDispatcher dispatcher(event);
        dispatcher.Dispatch<WindowCloseEvent>(
            [this](WindowCloseEvent&)
            {
                Close();
                return true;
            });
        dispatcher.Dispatch<WindowMinimizeEvent>(
            [this](WindowMinimizeEvent& minimize)
            {
                m_Minimized = minimize.IsMinimized();
                return false;
            });

        m_LayerStack.OnEvent(event);
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
