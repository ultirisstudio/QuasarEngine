#include <quasar/core/Application.h>
#include <quasar/core/EntryPoint.h>
#include <quasar/core/Input.h>
#include <quasar/core/Layer.h>
#include <quasar/core/Log.h>
#include <quasar/events/Event.h>

#include <string>
#include <utility>
#include <vector>

namespace
{
    class SandboxLayer : public Quasar::Layer
    {
    public:
        SandboxLayer() : Layer("Sandbox") {}

        void OnAttach() override { QUASAR_INFO("Layer '{}' attached", GetName()); }

        void OnDetach() override { QUASAR_INFO("Layer '{}' detached", GetName()); }

        void OnUpdate(Quasar::Timestep timestep) override
        {
            m_ElapsedSeconds += timestep.GetSeconds();
            ++m_FrameCount;

            if (m_ElapsedSeconds >= 1.0f)
            {
                QUASAR_INFO("{} frames in {:.3f} s", m_FrameCount, m_ElapsedSeconds);
                m_ElapsedSeconds = 0.0f;
                m_FrameCount = 0;
            }

            const bool spaceHeld = Quasar::Input::IsKeyPressed(Quasar::Key::Space);
            if (spaceHeld != m_SpaceHeld)
            {
                m_SpaceHeld = spaceHeld;
                const glm::vec2 cursor = Quasar::Input::GetMousePosition();
                QUASAR_INFO("Space {} (polled), cursor at {}, {}", spaceHeld ? "held" : "released", cursor.x, cursor.y);
            }
        }

        void OnEvent(Quasar::Event& event) override
        {
            if (event.GetType() != Quasar::EventType::MouseMoved)
            {
                QUASAR_INFO("{}", event);
            }
        }

    private:
        float m_ElapsedSeconds = 0.0f;
        int m_FrameCount = 0;
        bool m_SpaceHeld = false;
    };
}

Quasar::Scope<Quasar::Application> Quasar::CreateApplication(std::vector<std::string> arguments)
{
    for (const std::string& argument : arguments)
    {
        QUASAR_INFO("Argument: '{}'", argument);
    }

    Quasar::Scope<Application> application = Application::Create({.Name = "Sandbox", .Arguments = std::move(arguments), .Window = {.Title = "Sandbox"}});
    if (application)
    {
        application->PushLayer(Quasar::CreateScope<SandboxLayer>());
    }
    
    return application;
}
