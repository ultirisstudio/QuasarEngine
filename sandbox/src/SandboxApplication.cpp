#include "GamepadPanel.h"

#include <quasar/core/Application.h>
#include <quasar/core/EntryPoint.h>
#include <quasar/core/Layer.h>
#include <quasar/core/PlatformDetection.h>
#include <quasar/events/Event.h>
#include <quasar/imgui/ImGuiLayer.h>
#include <quasar/input/Input.h>
#include <quasar/log/Log.h>
#include <quasar/renderer/GraphicsAPI.h>

#include <imgui.h>

#include <algorithm>
#include <cstddef>
#include <deque>
#include <string>
#include <utility>
#include <vector>

namespace
{
    class EventTestLayer : public Quasar::Layer
    {
    public:
        EventTestLayer() : Layer("Event test") {}

        void OnAttach() override { QUASAR_INFO("Layer '{}' attached", GetName()); }

        void OnDetach() override { QUASAR_INFO("Layer '{}' detached", GetName()); }

        void OnUpdate(Quasar::Timestep timestep) override
        {
            m_ElapsedSeconds += timestep.GetSeconds();
            ++m_FrameCount;

            if (m_ElapsedSeconds >= 1.0f)
            {
                m_FramesPerSecond = static_cast<float>(m_FrameCount) / m_ElapsedSeconds;
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
            if (event.GetType() == Quasar::EventType::MouseMoved && !m_LogMouseMoves)
            {
                return;
            }

            if (event.GetType() == Quasar::EventType::TouchMoved && !m_LogTouchMoves)
            {
                return;
            }

            if (event.GetType() == Quasar::EventType::GamepadAxisMoved && !m_LogGamepadAxes)
            {
                return;
            }

            if (event.GetType() == Quasar::EventType::GamepadFingerMoved && !m_LogGamepadFingerMoves)
            {
                return;
            }

            std::string text = event.ToString();

            m_RecentEvents.push_back(std::move(text));
            if (m_RecentEvents.size() > MaxRecentEvents)
            {
                m_RecentEvents.pop_front();
            }
        }

        void OnImGuiRender() override
        {
            ImGui::DockSpaceOverViewport(0, nullptr, ImGuiDockNodeFlags_PassthruCentralNode);

            if (m_ShowDemoWindow)
            {
                ImGui::ShowDemoWindow(&m_ShowDemoWindow);
            }

            DrawEventWindow();

            m_GamepadPanel.Draw();
        }

    private:
        static constexpr std::size_t MaxRecentEvents = 200;

        void DrawEventWindow()
        {
            ImGui::Begin("Events");

            ImGui::TextUnformatted(fmt::format("{:.0f} frames per second", m_FramesPerSecond).c_str());
            Quasar::Window& window = Quasar::Application::Get().GetWindow();
            bool vsync = window.IsVSync();
            if (ImGui::Checkbox("VSync", &vsync))
            {
                window.SetVSync(vsync);
            }
            bool fullscreen = window.IsFullscreen();
            if (ImGui::Checkbox("Fullscreen", &fullscreen))
            {
                window.SetFullscreen(fullscreen);
            }
            ImGui::Checkbox("Show the ImGui demo window", &m_ShowDemoWindow);
            ImGui::Checkbox("Log mouse moves", &m_LogMouseMoves);
            ImGui::Checkbox("Log touch moves", &m_LogTouchMoves);
            ImGui::Checkbox("Log gamepad axes", &m_LogGamepadAxes);
            ImGui::Checkbox("Log gamepad finger moves", &m_LogGamepadFingerMoves);

            if (Quasar::ImGuiLayer* imGuiLayer = Quasar::Application::Get().GetImGuiLayer(); imGuiLayer != nullptr)
            {
                if (ImGui::Checkbox("ImGui keeps the events it uses", &m_ImGuiBlocksEvents))
                {
                    imGuiLayer->SetBlockEvents(m_ImGuiBlocksEvents);
                }
            }

            ImGui::PushStyleColor(ImGuiCol_Text, ImGui::GetStyleColorVec4(ImGuiCol_TextDisabled));
            ImGui::PushTextWrapPos(0.0f);
            ImGui::TextUnformatted("ImGui keeps the keyboard while one of its windows has the focus. Click outside them to type here.");
            ImGui::PopTextWrapPos();
            ImGui::PopStyleColor();

            if (ImGui::Button("Clear"))
            {
                m_RecentEvents.clear();
            }

            ImGui::Separator();
            ImGui::BeginChild("Recent events");
            for (const std::string& text : m_RecentEvents)
            {
                ImGui::TextUnformatted(text.c_str());
            }

            if (ImGui::GetScrollY() >= ImGui::GetScrollMaxY())
            {
                ImGui::SetScrollHereY(1.0f);
            }
            ImGui::EndChild();

            ImGui::End();
        }

        float m_ElapsedSeconds = 0.0f;
        int m_FrameCount = 0;
        float m_FramesPerSecond = 0.0f;
        bool m_SpaceHeld = false;

        bool m_ShowDemoWindow = true;
        bool m_LogMouseMoves = false;
        bool m_LogTouchMoves = false;
        bool m_LogGamepadAxes = false;
        bool m_LogGamepadFingerMoves = false;
        bool m_ImGuiBlocksEvents = true;
        std::deque<std::string> m_RecentEvents;
        Quasar::Sandbox::GamepadPanel m_GamepadPanel;
    };
}

Quasar::Scope<Quasar::Application> Quasar::CreateApplication(std::vector<std::string> arguments)
{
    for (const std::string& argument : arguments)
    {
        QUASAR_INFO("Argument: '{}'", argument);
    }

    const bool vulkan = std::ranges::find(arguments, "--vulkan") != arguments.end();
    const GraphicsAPI graphics = vulkan ? GraphicsAPI::Vulkan : WindowSpecification{}.Graphics;

    Quasar::Scope<Application> application = Application::Create(
        {.Name = "Sandbox", .Arguments = std::move(arguments), .Window = {.Title = "Sandbox", .VSync = PlatformDetection::IsAndroid(), .Graphics = graphics}});
    if (application)
    {
        application->PushLayer(Quasar::CreateScope<EventTestLayer>());
    }

    return application;
}
