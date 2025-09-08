#include "qepch.h"
#include "DirectXImGuiLayer.h"

#include <GLFW/glfw3.h>

#include <imgui/imgui.h>
#include <ImGuizmo.h>

#include <backends/imgui_impl_dx11.h>
#include <backends/imgui_impl_glfw.h>

#include <QuasarEngine/Core/Application.h>

#include "DirectXContext.h"

namespace QuasarEngine
{
    DirectXImGuiLayer::DirectXImGuiLayer()
        : ImGuiLayer("DirectXImGuiLayer")
    {
    }

    DirectXImGuiLayer::~DirectXImGuiLayer()
    {
    }

    void DirectXImGuiLayer::OnAttach()
    {
        IMGUI_CHECKVERSION();
        ImGui::CreateContext();

        ImGuiIO& io = ImGui::GetIO(); (void)io;
        io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;
        io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;
        io.ConfigFlags |= ImGuiConfigFlags_ViewportsEnable;

        float fontSize = 14.0f;
        io.Fonts->AddFontFromFileTTF("Assets/Fonts/Monocraft.ttf", fontSize);
        io.FontDefault = io.Fonts->AddFontFromFileTTF("Assets/Fonts/Monocraft.ttf", fontSize);

        ImGui::StyleColorsRealDark();

        ImGuiStyle& style = ImGui::GetStyle();
        if (io.ConfigFlags & ImGuiConfigFlags_ViewportsEnable)
        {
            style.WindowRounding = 0.0f;
            style.Colors[ImGuiCol_WindowBg].w = 1.0f;
        }

        Application& app = Application::Get();
        m_Window = static_cast<GLFWwindow*>(app.GetWindow().GetNativeWindow());

        auto& ctx = DirectXContext::Context;
        m_Device = ctx.device.Get();
        m_DeviceContext = ctx.deviceContext.Get();

        ImGui_ImplGlfw_InitForOther(m_Window, true);
        ImGui_ImplDX11_Init(m_Device, m_DeviceContext);
    }

    void DirectXImGuiLayer::OnDetach()
    {
        ImGui_ImplDX11_Shutdown();
        ImGui_ImplGlfw_Shutdown();
        ImGui::DestroyContext();
    }

    void DirectXImGuiLayer::Begin()
    {
        ImGui_ImplDX11_NewFrame();
        ImGui_ImplGlfw_NewFrame();
        ImGui::NewFrame();
        ImGuizmo::BeginFrame();
    }

    void DirectXImGuiLayer::End()
    {
        ImGuiIO& io = ImGui::GetIO();
        Application& app = Application::Get();
        io.DisplaySize = ImVec2((float)app.GetWindow().GetWidth(), (float)app.GetWindow().GetHeight());

        ImGui::Render();
        ImGui_ImplDX11_RenderDrawData(ImGui::GetDrawData());

        if (io.ConfigFlags & ImGuiConfigFlags_ViewportsEnable)
        {
            ImGui::UpdatePlatformWindows();
            ImGui::RenderPlatformWindowsDefault();
        }
    }
}
