#include <EditorContext.h>
#include <panels/GamepadPanel.h>

#include <quasar/core/Base.h>
#include <quasar/events/Event.h>
#include <quasar/input/GamepadBackend.h>
#include <quasar/input/GamepadCodes.h>
#include <quasar/input/Gamepads.h>

#include <doctest/doctest.h>

#include <imgui.h>
#include <imgui_internal.h>

#include <chrono>
#include <cstddef>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

using Quasar::GamepadId;
using Quasar::Editor::EditorContext;
using Quasar::Editor::GamepadPanel;

namespace
{
    constexpr GamepadId First{0};
    constexpr GamepadId Second{1};
    constexpr GamepadId Third{2};
    constexpr GamepadId Fourth{3};

    struct RecordedRumble
    {
        GamepadId Gamepad{};
        Quasar::GamepadRumble Rumble;
    };

    class FakeBackend final : public Quasar::GamepadBackend
    {
    public:
        FakeBackend(Quasar::GamepadReadings& readings, std::vector<RecordedRumble>& rumbles, bool& canRumble)
            : m_Readings(&readings), m_Rumbles(&rumbles), m_CanRumble(&canRumble)
        {
        }

        Quasar::GamepadReadings Read() override { return *m_Readings; }

        bool Rumble(GamepadId gamepad, const Quasar::GamepadRumble& rumble) override
        {
            m_Rumbles->push_back({.Gamepad = gamepad, .Rumble = rumble});
            return *m_CanRumble;
        }

    private:
        Quasar::GamepadReadings* m_Readings;
        std::vector<RecordedRumble>* m_Rumbles;
        bool* m_CanRumble;
    };

    class PanelTest
    {
    public:
        PanelTest()
            : m_Gamepads(Quasar::CreateScope<FakeBackend>(m_Readings, m_Rumbles, m_CanRumble)), m_Panel(m_Context, m_Gamepads),
              m_Previous(ImGui::GetCurrentContext()), m_ImGui(ImGui::CreateContext())
        {
            ImGui::SetCurrentContext(m_ImGui);
            ImGuiIO& io = ImGui::GetIO();
            io.DisplaySize = ImVec2(1600.0f, 1000.0f);
            io.DeltaTime = 1.0f / 60.0f;
            io.IniFilename = nullptr;
            io.BackendFlags |= ImGuiBackendFlags_RendererHasTextures;
        }

        ~PanelTest()
        {
            ImGui::DestroyContext(m_ImGui);
            ImGui::SetCurrentContext(m_Previous);
        }

        PanelTest(const PanelTest&) = delete;
        PanelTest& operator=(const PanelTest&) = delete;
        PanelTest(PanelTest&&) = delete;
        PanelTest& operator=(PanelTest&&) = delete;

        void Connect(GamepadId gamepad, std::string name)
        {
            Quasar::GamepadReading reading;
            reading.Name = std::move(name);
            m_Readings.at(static_cast<std::size_t>(gamepad)) = reading;
            m_Gamepads.Update([](Quasar::Event& /*event*/) {});
        }

        void Disconnect(GamepadId gamepad)
        {
            m_Readings.at(static_cast<std::size_t>(gamepad)).reset();
            m_Gamepads.Update([](Quasar::Event& /*event*/) {});
        }

        void SetCanRumble(bool canRumble) { m_CanRumble = canRumble; }

        void Frame(bool large = true)
        {
            ImGui::NewFrame();
            if (large)
            {
                ImGui::SetNextWindowPos(ImVec2(0.0f, 0.0f));
                ImGui::SetNextWindowSize(ImVec2(1500.0f, 900.0f));
            }
            m_Panel.Draw();
            ImGui::Render();
            for (ImTextureData* texture : ImGui::GetPlatformIO().Textures)
            {
                if (texture->Status == ImTextureStatus_WantCreate || texture->Status == ImTextureStatus_WantUpdates)
                {
                    texture->SetTexID(1);
                    texture->SetStatus(ImTextureStatus_OK);
                }
            }
        }

        void ClickButton(const char* label)
        {
            Frame();
            ImGuiWindow* details = FindDetails();
            REQUIRE(details != nullptr);
            ImGui::ActivateItemByID(details->GetID(label));
            Frame();
            Frame();
        }

        const std::vector<RecordedRumble>& GetRumbles() const { return m_Rumbles; }

    private:
        static ImGuiWindow* FindDetails()
        {
            for (ImGuiWindow* window : ImGui::GetCurrentContext()->Windows)
            {
                if (std::string_view(window->Name).starts_with("Gamepad/Details"))
                {
                    return window;
                }
            }
            return nullptr;
        }

        Quasar::GamepadReadings m_Readings{};
        std::vector<RecordedRumble> m_Rumbles;
        bool m_CanRumble = true;
        Quasar::Gamepads m_Gamepads;
        EditorContext m_Context;
        GamepadPanel m_Panel;
        ImGuiContext* m_Previous;
        ImGuiContext* m_ImGui;
    };
}

TEST_CASE("The gamepad panel opens at its initial size and draws without a gamepad")
{
    PanelTest test;

    test.Frame(false);

    const ImGuiWindow* window = ImGui::FindWindowByName("Gamepad");
    REQUIRE(window != nullptr);
    CHECK(window->Size.x > 1000.0f);
    CHECK(window->Size.y > 300.0f);
}

TEST_CASE("The rumble buttons of the gamepad panel rumble the first connected gamepad")
{
    PanelTest test;
    test.Connect(Second, "Second gamepad");
    test.Connect(Third, "Third gamepad");

    test.ClickButton("Light");
    test.ClickButton("Strong");
    test.ClickButton("Stop");

    const std::vector<RecordedRumble>& rumbles = test.GetRumbles();
    REQUIRE(rumbles.size() == 3);
    for (const RecordedRumble& rumble : rumbles)
    {
        CHECK(rumble.Gamepad == Second);
    }
    CHECK(rumbles[0].Rumble.LowFrequency == 0.25f);
    CHECK(rumbles[0].Rumble.Duration == std::chrono::milliseconds(300));
    CHECK(rumbles[1].Rumble.HighFrequency == 1.0f);
    CHECK(rumbles[1].Rumble.Duration == std::chrono::milliseconds(1000));
    CHECK(rumbles[2].Rumble.LowFrequency == 0.0f);
    CHECK(rumbles[2].Rumble.HighFrequency == 0.0f);
}

TEST_CASE("The gamepad panel selects another connected gamepad when the selected one disconnects")
{
    PanelTest test;
    test.Connect(First, "First gamepad");
    test.Connect(Fourth, "Fourth gamepad");
    test.Frame();

    test.Disconnect(First);
    test.ClickButton("Light");
    test.Disconnect(Fourth);
    test.Frame();

    REQUIRE(test.GetRumbles().size() == 1);
    CHECK(test.GetRumbles()[0].Gamepad == Fourth);
}
