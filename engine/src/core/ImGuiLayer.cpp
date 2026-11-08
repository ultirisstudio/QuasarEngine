#include <quasar/core/ImGuiLayer.h>
#include <quasar/core/ImGuiTheme.h>
#include <quasar/core/Log.h>
#include <quasar/core/Window.h>

#include <imgui.h>
#include <imgui_impl_opengl3.h>
#include <imgui_impl_sdl3.h>

#include <SDL3/SDL_events.h>
#include <SDL3/SDL_video.h>

namespace Quasar
{
    ImGuiLayer::ImGuiLayer(Window& window) : Layer("ImGui"), m_Window(&window) {}

    void ImGuiLayer::OnAttach()
    {
        IMGUI_CHECKVERSION();
        ImGui::CreateContext();

        ImGuiIO& io = ImGui::GetIO();
        io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;
        io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;
        io.ConfigFlags |= ImGuiConfigFlags_ViewportsEnable;

        SDL_Window* nativeWindow = m_Window->GetNativeWindow();
        const bool platformStarted = ImGui_ImplSDL3_InitForOpenGL(nativeWindow, m_Window->GetNativeContext());
        if (!platformStarted)
        {
            QUASAR_ENGINE_ERROR("Cannot start the SDL backend of ImGui");
        }
        m_Window->SetNativeEventCallback([](const SDL_Event& event) { ImGui_ImplSDL3_ProcessEvent(&event); });
        if (!ImGui_ImplOpenGL3_Init("#version 450 core"))
        {
            QUASAR_ENGINE_ERROR("Cannot start the OpenGL backend of ImGui");
        }

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
        ImGui_ImplOpenGL3_Shutdown();
        ImGui_ImplSDL3_Shutdown();
        ImGui::DestroyContext();
    }

    void ImGuiLayer::OnEvent(Event& event)
    {
        if (!m_BlockEvents)
        {
            return;
        }

        const ImGuiIO& io = ImGui::GetIO();
        if ((event.IsInCategory(EventCategory::Mouse) && io.WantCaptureMouse) || (event.IsInCategory(EventCategory::Keyboard) && io.WantCaptureKeyboard))
        {
            event.SetHandled(true);
        }
    }

    // ImGui keeps its state in a global context, but Begin() and End() stay on the layer that owns it
    // NOLINTNEXTLINE(readability-convert-member-functions-to-static)
    void ImGuiLayer::Begin()
    {
        ImGui_ImplOpenGL3_NewFrame();
        ImGui_ImplSDL3_NewFrame();
        ImGui::NewFrame();
    }

    // NOLINTNEXTLINE(readability-convert-member-functions-to-static)
    void ImGuiLayer::End()
    {
        ImGui::Render();
        ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

        if ((ImGui::GetIO().ConfigFlags & ImGuiConfigFlags_ViewportsEnable) != 0)
        {
            SDL_Window* mainWindow = SDL_GL_GetCurrentWindow();
            SDL_GLContext mainContext = SDL_GL_GetCurrentContext();
            ImGui::UpdatePlatformWindows();
            ImGui::RenderPlatformWindowsDefault();
            SDL_GL_MakeCurrent(mainWindow, mainContext);
        }
    }
}
