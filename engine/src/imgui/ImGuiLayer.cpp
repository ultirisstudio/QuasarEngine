#include <quasar/core/Window.h>
#include <quasar/imgui/ImGuiLayer.h>
#include <quasar/imgui/ImGuiTheme.h>
#include <quasar/log/Log.h>

#include <imgui/ImGuiBackend.h>

#include <imgui.h>
#include <imgui_impl_sdl3.h>

#include <SDL3/SDL_events.h>
#include <SDL3/SDL_video.h>

namespace Quasar
{
    ImGuiLayer::ImGuiLayer(Window& window) : Layer("ImGui"), m_Window(&window) {}

    ImGuiLayer::~ImGuiLayer() = default;

    void ImGuiLayer::OnAttach()
    {
        IMGUI_CHECKVERSION();
        ImGui::CreateContext();

        ImGuiIO& io = ImGui::GetIO();
        io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;
        io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;
        io.ConfigFlags |= ImGuiConfigFlags_ViewportsEnable;

        m_Backend = ImGuiBackend::Create(m_Window->GetGraphicsAPI());
        if (m_Backend != nullptr && m_Backend->Init(*m_Window))
        {
            m_Window->SetNativeEventCallback([](const SDL_Event& event) { ImGui_ImplSDL3_ProcessEvent(&event); });
        }
        else
        {
            m_Backend.reset();
        }

        SDL_Window* nativeWindow = m_Window->GetNativeWindow();

        io.ConfigDpiScaleFonts = true;
        io.ConfigDpiScaleViewports = true;
        const float dpiScale = SDL_GetWindowDisplayScale(nativeWindow);
        ImGuiTheme::ApplyStyle(ImGui::GetStyle(), dpiScale);
        ImGuiTheme::LoadFont(*io.Fonts, ImGuiTheme::GetFontPath());

        if ((io.BackendFlags & ImGuiBackendFlags_PlatformHasViewports) == 0)
        {
            io.ConfigFlags &= ~ImGuiConfigFlags_ViewportsEnable;
            QUASAR_ENGINE_WARN("ImGui windows cannot leave the main window on this platform");
        }

        QUASAR_ENGINE_INFO("Started Dear ImGui {}", ImGui::GetVersion());
    }

    void ImGuiLayer::OnDetach()
    {
        m_Window->SetNativeEventCallback({});
        if (m_Backend != nullptr)
        {
            m_Backend->Shutdown();
            m_Backend.reset();
        }
        ImGui::DestroyContext();
    }

    void ImGuiLayer::OnEvent(Event& event)
    {
        if (!m_BlockEvents)
        {
            return;
        }

        const ImGuiIO& io = ImGui::GetIO();
        const bool pointer = event.IsInCategory(EventCategory::Mouse) || event.IsInCategory(EventCategory::Touch);
        if ((pointer && io.WantCaptureMouse) || (event.IsInCategory(EventCategory::Keyboard) && io.WantCaptureKeyboard))
        {
            event.SetHandled(true);
        }
    }

    void ImGuiLayer::Begin()
    {
        if (m_Backend != nullptr)
        {
            m_Backend->NewFrame();
        }
        ImGui::NewFrame();
    }

    void ImGuiLayer::End()
    {
        ImGui::Render();
        if (m_Backend != nullptr)
        {
            m_Backend->Render();
        }
    }
}
