#include <quasar/core/Application.h>
#include <quasar/core/Layer.h>
#include <quasar/core/Log.h>

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
        }

    private:
        float m_ElapsedSeconds = 0.0f;
        int m_FrameCount = 0;
    };
}

int main()
{
    Quasar::Log::Init();

    int exitCode = 0;
    {
        Quasar::Scope<Quasar::Application> application = Quasar::Application::Create({.Window = {.Title = "Sandbox"}});
        if (application)
        {
            application->PushLayer(Quasar::CreateScope<SandboxLayer>());
            application->Run();
        }
        else
        {
            exitCode = 1;
        }
    }

    Quasar::Log::Shutdown();
    return exitCode;
}
