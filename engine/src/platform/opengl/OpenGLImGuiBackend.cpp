#include <platform/opengl/OpenGLImGuiBackend.h>

#include <quasar/core/Window.h>
#include <quasar/log/Log.h>
#include <quasar/platform/opengl/OpenGLContext.h>

#include <imgui.h>
#include <imgui_impl_opengl3.h>
#include <imgui_impl_sdl3.h>

#include <SDL3/SDL_video.h>

namespace Quasar
{
    bool OpenGLImGuiBackend::Init(Window& window)
    {
        const auto* context = dynamic_cast<const OpenGLContext*>(window.GetGraphicsContext());
        if (context == nullptr)
        {
            QUASAR_ENGINE_ERROR("Cannot start the OpenGL backend of ImGui: the window has no OpenGL context");
            return false;
        }

        if (!ImGui_ImplSDL3_InitForOpenGL(window.GetNativeWindow(), context->GetNativeContext()))
        {
            QUASAR_ENGINE_ERROR("Cannot start the SDL backend of ImGui");
            return false;
        }
        if (!ImGui_ImplOpenGL3_Init("#version 450 core"))
        {
            QUASAR_ENGINE_ERROR("Cannot start the OpenGL backend of ImGui");
            ImGui_ImplSDL3_Shutdown();
            return false;
        }
        return true;
    }

    void OpenGLImGuiBackend::Shutdown()
    {
        ImGui_ImplOpenGL3_Shutdown();
        ImGui_ImplSDL3_Shutdown();
    }

    void OpenGLImGuiBackend::NewFrame()
    {
        ImGui_ImplOpenGL3_NewFrame();
        ImGui_ImplSDL3_NewFrame();
    }

    void OpenGLImGuiBackend::Render()
    {
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
