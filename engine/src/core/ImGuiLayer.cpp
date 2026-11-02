#include <quasar/core/ImGuiLayer.h>
#include <quasar/core/Log.h>
#include <quasar/core/Window.h>

#include <imgui.h>
#include <imgui_impl_glfw.h>
#include <imgui_impl_opengl3.h>

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
        ImGui::StyleColorsDark();

        if (!ImGui_ImplGlfw_InitForOpenGL(m_Window->GetNativeWindow(), true))
        {
            QUASAR_ENGINE_ERROR("Cannot start the GLFW backend of ImGui");
        }
        if (!ImGui_ImplOpenGL3_Init("#version 450 core"))
        {
            QUASAR_ENGINE_ERROR("Cannot start the OpenGL backend of ImGui");
        }

        QUASAR_ENGINE_INFO("Started Dear ImGui {}", ImGui::GetVersion());
    }

    void ImGuiLayer::OnDetach()
    {
        ImGui_ImplOpenGL3_Shutdown();
        ImGui_ImplGlfw_Shutdown();
        ImGui::DestroyContext();
    }

    void ImGuiLayer::OnEvent(Event& event)
    {
        if (!m_BlockEvents)
        {
            return;
        }

        const ImGuiIO& io = ImGui::GetIO();
        if ((event.IsInCategory(EventCategory::Mouse) && io.WantCaptureMouse) ||
            (event.IsInCategory(EventCategory::Keyboard) && io.WantCaptureKeyboard))
        {
            event.SetHandled(true);
        }
    }

    // ImGui keeps its state in a global context, but Begin() and End() stay on the layer that owns it
    // NOLINTNEXTLINE(readability-convert-member-functions-to-static)
    void ImGuiLayer::Begin()
    {
        ImGui_ImplOpenGL3_NewFrame();
        ImGui_ImplGlfw_NewFrame();
        ImGui::NewFrame();
    }

    // NOLINTNEXTLINE(readability-convert-member-functions-to-static)
    void ImGuiLayer::End()
    {
        ImGui::Render();
        ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
    }
}