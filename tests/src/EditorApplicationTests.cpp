#include "NullWindow.h"

#include <EditorApplication.h>

#include <quasar/core/Application.h>
#include <quasar/core/Layer.h>
#include <quasar/core/Log.h>
#include <quasar/core/MemoryLogSink.h>
#include <quasar/core/PlatformDetection.h>

#include <doctest/doctest.h>

#include <imgui.h>

#include <algorithm>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

namespace
{
    bool HasMessage(std::string_view text)
    {
        const std::vector<Quasar::LogMessage> messages = Quasar::Log::GetMemorySink()->GetMessages();
        return std::ranges::any_of(messages, [text](const Quasar::LogMessage& message) { return message.Text == text; });
    }
}

TEST_CASE("The editor application has its own name, window and arguments")
{
    const Quasar::ApplicationSpecification specification = Quasar::Editor::GetEditorSpecification({"--project", "Game"});

    CHECK(specification.Name == "Editor");
    CHECK(specification.Window.Title == "Quasar Editor");
    CHECK(specification.Window.Width == 1600);
    CHECK(specification.Window.Height == 900);
    CHECK(specification.Window.GraphicsContext);
    CHECK(specification.Arguments == std::vector<std::string>{"--project", "Game"});
}

// The editor needs an OpenGL context: this test needs the Mesa software driver through EGL, only on Linux
#ifdef QUASAR_PLATFORM_LINUX

namespace
{
    class ClosingLayer : public Quasar::Layer
    {
    public:
        ClosingLayer() : Layer("Closing") {}

        void OnUpdate(Quasar::Timestep /*timestep*/) override
        {
            ++Updates;
            if (Updates == 3)
            {
                Quasar::Application::Get().Close();
            }
        }

        int Updates = 0;
    };
}

TEST_CASE("The editor application opens its window and runs the editor layer")
{
    const TestHelpers::ScopedLog log;
    {
        TestHelpers::UseSoftwareOpenGL();
        const Quasar::Scope<Quasar::Application> application = Quasar::Editor::CreateEditorApplication({});
        REQUIRE(application != nullptr);

        ImGui::GetIO().IniFilename = nullptr;
        CHECK(application->GetWindow().GetTitle() == "Quasar Editor");
        CHECK(HasMessage("Editor started"));

        auto layer = Quasar::CreateScope<ClosingLayer>();
        const ClosingLayer& closing = *layer;
        application->PushLayer(std::move(layer));

        application->Run();

        CHECK(closing.Updates == 3);
        CHECK_FALSE(HasMessage("Editor stopped"));
    }

    CHECK(HasMessage("Editor stopped"));
}

#endif
