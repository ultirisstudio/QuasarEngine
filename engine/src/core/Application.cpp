#include <quasar/core/Application.h>
#include <quasar/events/EventDispatcher.h>
#include <quasar/events/WindowEvent.h>
#include <quasar/io/UserDataDirectory.h>
#include <quasar/log/Log.h>
#include <quasar/renderer/GraphicsAPI.h>
#include <quasar/renderer/GraphicsContext.h>

#include <spdlog/fmt/std.h>

#include <algorithm>
#include <filesystem>
#include <optional>
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

        if (m_Window->GetGraphicsAPI() != GraphicsAPI::None)
        {
            m_ImGuiLayer = CreateScope<ImGuiLayer>(*m_Window);
            m_ImGuiLayer->OnAttach();
        }
    }

    Application::~Application()
    {
        m_DeferLayerChanges = true;
        m_LayerStack.Clear();
        m_LayerChanges.clear();

        if (m_ImGuiLayer)
        {
            m_ImGuiLayer->OnDetach();
            m_ImGuiLayer.reset();
        }

        m_Window->SetEventCallback({});
        s_Instance = nullptr;
    }

    Scope<Application> Application::Create(const ApplicationSpecification& specification)
    {
        if (s_Instance != nullptr)
        {
            QUASAR_ENGINE_ERROR("Cannot create the application '{}': an application already exists", specification.Name);
            return nullptr;
        }

        if (!specification.WorkingDirectory.empty())
        {
            std::error_code error;
            std::filesystem::current_path(specification.WorkingDirectory, error);
            if (error)
            {
                QUASAR_ENGINE_ERROR("Cannot create the application '{}': cannot use '{}' as working directory: {}", specification.Name,
                                    specification.WorkingDirectory, error.message());
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

    std::optional<std::filesystem::path> Application::GetUserDataDirectory() const
    {
        return Quasar::GetUserDataDirectory(m_Specification.Organization, m_Specification.Name);
    }

    void Application::Run()
    {
        m_Running = true;
        m_DeferLayerChanges = true;
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

            GraphicsContext* context = m_Window->GetGraphicsContext();
            const bool canRender = !m_Minimized && m_Window->GetWidth() > 0 && m_Window->GetHeight() > 0 && (context == nullptr || context->BeginFrame());
            if (canRender)
            {
                for (Scope<Layer>& layer : m_LayerStack)
                {
                    layer->OnRender();
                }

                if (m_ImGuiLayer)
                {
                    m_ImGuiLayer->Begin();
                    for (Scope<Layer>& layer : m_LayerStack)
                    {
                        layer->OnImGuiRender();
                    }
                    m_ImGuiLayer->End();
                }
            }

            m_Window->Update();
            m_Gamepads.Update([this](Event& event) { OnEvent(event); });
            ApplyLayerChanges();
        }

        m_DeferLayerChanges = false;
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

        if (m_ImGuiLayer && !event.IsHandled())
        {
            m_ImGuiLayer->OnEvent(event);
        }
        m_LayerStack.OnEvent(event);
    }

    void Application::Close()
    {
        m_Running = false;
    }

    Layer& Application::PushLayer(Scope<Layer> layer)
    {
        if (!m_DeferLayerChanges)
        {
            return m_LayerStack.PushLayer(std::move(layer));
        }

        QUASAR_ENGINE_ASSERT(layer != nullptr, "Cannot push an empty layer");
        Layer& added = *layer;
        m_LayerChanges.push_back({.Kind = LayerChange::Type::PushLayer, .Added = std::move(layer)});
        return added;
    }

    Layer& Application::PushOverlay(Scope<Layer> overlay)
    {
        if (!m_DeferLayerChanges)
        {
            return m_LayerStack.PushOverlay(std::move(overlay));
        }

        QUASAR_ENGINE_ASSERT(overlay != nullptr, "Cannot push an empty overlay");
        Layer& added = *overlay;
        m_LayerChanges.push_back({.Kind = LayerChange::Type::PushOverlay, .Added = std::move(overlay)});
        return added;
    }

    void Application::PopLayer(Layer& layer)
    {
        if (!m_DeferLayerChanges)
        {
            m_LayerStack.PopLayer(layer);
            return;
        }

        if (!IsRemovalPending(layer))
        {
            m_LayerChanges.push_back({.Kind = LayerChange::Type::PopLayer, .Removed = &layer});
        }
    }

    void Application::PopOverlay(Layer& overlay)
    {
        if (!m_DeferLayerChanges)
        {
            m_LayerStack.PopOverlay(overlay);
            return;
        }

        if (!IsRemovalPending(overlay))
        {
            m_LayerChanges.push_back({.Kind = LayerChange::Type::PopOverlay, .Removed = &overlay});
        }
    }

    bool Application::IsRemovalPending(const Layer& layer) const
    {
        return std::ranges::any_of(m_LayerChanges, [&layer](const LayerChange& change) { return change.Removed == &layer; });
    }

    void Application::ApplyLayerChanges()
    {
        while (!m_LayerChanges.empty())
        {
            std::vector<LayerChange> changes = std::exchange(m_LayerChanges, {});
            for (LayerChange& change : changes)
            {
                switch (change.Kind)
                {
                    case LayerChange::Type::PushLayer:
                        m_LayerStack.PushLayer(std::move(change.Added));
                        break;
                    case LayerChange::Type::PushOverlay:
                        m_LayerStack.PushOverlay(std::move(change.Added));
                        break;
                    case LayerChange::Type::PopLayer:
                        m_LayerStack.PopLayer(*change.Removed);
                        break;
                    case LayerChange::Type::PopOverlay:
                        m_LayerStack.PopOverlay(*change.Removed);
                        break;
                }
            }
        }
    }
}
