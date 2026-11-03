#include "LogHelpers.h"
#include "NullWindow.h"
#include "TestLayers.h"

#include <EditorApplication.h>
#include <EditorLayer.h>

#include <quasar/core/Application.h>
#include <quasar/core/Layer.h>
#include <quasar/core/MemoryLogSink.h>
#include <quasar/core/PlatformDetection.h>

#include <doctest/doctest.h>

#include <imgui.h>

#include <algorithm>
#include <string>
#include <utility>
#include <vector>

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
    bool HasEditorLayer(const Quasar::Application& application)
    {
        return std::ranges::any_of(application.GetLayerStack(), [](const Quasar::Scope<Quasar::Layer>& layer)
                                   { return dynamic_cast<const Quasar::Editor::EditorLayer*>(layer.get()) != nullptr; });
    }
}

TEST_CASE("The editor application opens its window and runs the editor layer")
{
    const TestHelpers::ScopedLog log;
    TestHelpers::UseSoftwareOpenGL();
    const Quasar::Scope<Quasar::Application> application = Quasar::Editor::CreateEditorApplication({});
    REQUIRE(application != nullptr);

    ImGui::GetIO().IniFilename = nullptr;
    auto layer = Quasar::CreateScope<TestHelpers::ClosingLayer>();
    const TestHelpers::ClosingLayer& closing = *layer;
    application->PushLayer(std::move(layer));

    application->Run();

    CHECK(application->GetWindow().GetTitle() == "Quasar Editor");
    CHECK(HasEditorLayer(*application));
    CHECK(closing.GetUpdates() == 3);
    CHECK_FALSE(TestHelpers::HasLogAtLeast(Quasar::LogLevel::Error));
}

#endif