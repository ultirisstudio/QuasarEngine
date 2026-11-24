#include "helpers/LogHelpers.h"
#include "helpers/NullWindow.h"
#include "helpers/ScopedEnvironmentVariable.h"
#include "helpers/TemporaryFiles.h"
#include "helpers/TestLayers.h"

#include <EditorApplication.h>
#include <EditorLayer.h>
#include <EditorSettings.h>

#include <quasar/core/Application.h>
#include <quasar/core/Layer.h>
#include <quasar/core/PlatformDetection.h>
#include <quasar/log/MemoryLogSink.h>
#include <quasar/renderer/GraphicsAPI.h>

#include <doctest/doctest.h>

#include <imgui.h>

#include <algorithm>
#include <filesystem>
#include <string>
#include <utility>
#include <vector>

namespace
{
    bool HasEditorLayer(const Quasar::Application& application)
    {
        return std::ranges::any_of(application.GetLayerStack(), [](const Quasar::Scope<Quasar::Layer>& layer)
                                   { return dynamic_cast<const Quasar::Editor::EditorLayer*>(layer.get()) != nullptr; });
    }
}

TEST_CASE("The editor application has a graphics API and keeps its arguments")
{
    const Quasar::ApplicationSpecification specification = Quasar::Editor::GetEditorSpecification({"--project", "Game"});

    CHECK(specification.Window.Graphics != Quasar::GraphicsAPI::None);
    CHECK(specification.Arguments == std::vector<std::string>{"--project", "Game"});
}

TEST_CASE("The editor application opens its window and runs the editor layer")
{
    const TestHelpers::ScopedLog log;
    TestHelpers::UseTestVideoDriver();
    const Quasar::Scope<Quasar::Application> application = Quasar::Editor::CreateEditorApplication({});
    REQUIRE(application != nullptr);

    ImGui::GetIO().IniFilename = nullptr;
    auto layer = Quasar::CreateScope<TestHelpers::ClosingLayer>();
    const TestHelpers::ClosingLayer& closing = *layer;
    application->PushLayer(std::move(layer));

    application->Run();

    CHECK(HasEditorLayer(*application));
    CHECK(closing.GetUpdates() == 3);
    CHECK_FALSE(TestHelpers::HasLogAtLeast(Quasar::LogLevel::Error));
}

#ifdef QUASAR_PLATFORM_LINUX
TEST_CASE("The editor keeps its settings in the user data folder of the editor")
{
    const TestHelpers::ScopedLog log;
    const TestHelpers::TemporaryFolder folder("quasar_editor_user_data");
    const TestHelpers::ScopedEnvironmentVariable dataHome("XDG_DATA_HOME", folder.GetPath().string());
    TestHelpers::UseTestVideoDriver();
    const Quasar::Scope<Quasar::Application> application = Quasar::Editor::CreateEditorApplication({});
    REQUIRE(application != nullptr);

    CHECK(Quasar::Editor::GetEditorSettingsFile() == folder.GetPath() / "Ultiris Studio" / "Quasar Editor" / "EditorSettings.yaml");
}
#endif
