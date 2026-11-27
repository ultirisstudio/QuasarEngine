#include "ControlsPanel.h"

#include <quasar/core/Application.h>
#include <quasar/core/Window.h>
#include <quasar/input/KeyCodes.h>

#include <imgui.h>
#include <spdlog/fmt/fmt.h>

#include <algorithm>
#include <cctype>
#include <string>

namespace Quasar::Sandbox
{
    namespace
    {
        std::string KeyLabel(Key key)
        {
            std::string name = Application::Get().GetWindow().GetKeyName(key);
            std::ranges::transform(name, name.begin(), [](unsigned char character) { return static_cast<char>(std::toupper(character)); });
            return name;
        }
    }

    void DrawControlsPanel()
    {
        const ImGuiViewport* viewport = ImGui::GetMainViewport();
        const ImGuiCond placement = ImGui::GetFrameCount() < 3 ? ImGuiCond_Always : ImGuiCond_FirstUseEver;
        ImGui::SetNextWindowPos({viewport->WorkPos.x + 10.0f, viewport->WorkPos.y + viewport->WorkSize.y - 10.0f}, placement, {0.0f, 1.0f});
        ImGui::SetNextWindowCollapsed(true, ImGuiCond_FirstUseEver);
        if (ImGui::Begin("Controls", nullptr, ImGuiWindowFlags_AlwaysAutoResize))
        {
            ImGui::SeparatorText("Keyboard and mouse");
            ImGui::TextUnformatted(fmt::format("Move: {} {} {} {}", KeyLabel(Key::W), KeyLabel(Key::A), KeyLabel(Key::S), KeyLabel(Key::D)).c_str());
            ImGui::TextUnformatted("Up and down: Space and Left Ctrl");
            ImGui::TextUnformatted("Faster: Left Shift");
            ImGui::TextUnformatted("Look around: hold the right mouse button");
            ImGui::TextUnformatted("Fullscreen: F11");
            ImGui::SeparatorText("Gamepad");
            ImGui::TextUnformatted("Left stick to move, right stick to look");
            ImGui::TextUnformatted("Triggers to go up and down, left stick click to go faster");
        }
        ImGui::End();
    }
}
