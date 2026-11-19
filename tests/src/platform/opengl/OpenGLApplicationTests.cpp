#include "helpers/LogHelpers.h"
#include "helpers/NullWindow.h"

#include <quasar/core/Application.h>
#include <quasar/core/Layer.h>
#include <quasar/core/Window.h>
#include <quasar/log/MemoryLogSink.h>
#include <quasar/renderer/GraphicsContext.h>
#include <quasar/renderer/PipelineLibrary.h>
#include <quasar/time/Timestep.h>

#include <doctest/doctest.h>

#include <glad/gl.h>
#include <glm/vec4.hpp>

#include <array>
#include <cstdint>
#include <optional>
#include <utility>

namespace
{
    using Color = std::array<int, 4>;

    class PixelLayer : public Quasar::Layer
    {
    public:
        PixelLayer() : Layer("Pixel") {}

        void OnUpdate(Quasar::Timestep /*timestep*/) override
        {
            Quasar::Application::Get().GetWindow().GetGraphicsContext()->ClearWindow({0.0f, 1.0f, 0.0f, 1.0f});
        }

        void OnRender() override
        {
            Quasar::Application::Get().GetWindow().GetGraphicsContext()->BindWindow();
            std::array<std::uint8_t, 4> pixel{};
            glReadPixels(0, 0, 1, 1, GL_RGBA, GL_UNSIGNED_BYTE, pixel.data());
            Pixel = {pixel[0], pixel[1], pixel[2], pixel[3]};
            Quasar::Application::Get().Close();
        }

        Color Pixel{};
    };

    Color RenderPixel(const std::optional<glm::vec4>& clearColor)
    {
        const Quasar::Scope<Quasar::Application> application = TestHelpers::CreateApplicationWithContext({.ClearColor = clearColor});
        REQUIRE(application != nullptr);
        Quasar::Scope<PixelLayer> layer = Quasar::CreateScope<PixelLayer>();
        const PixelLayer& added = *layer;
        application->PushLayer(std::move(layer));
        application->Run();
        return added.Pixel;
    }
}

TEST_CASE("An application with a graphics context creates a renderer, a scene renderer and the default pipelines")
{
    const TestHelpers::ScopedLog log;
    const Quasar::Scope<Quasar::Application> application = TestHelpers::CreateApplicationWithContext();
    REQUIRE(application != nullptr);

    CHECK(application->GetRenderer() != nullptr);
    CHECK(application->GetSceneRenderer() != nullptr);
    REQUIRE(application->GetPipelines() != nullptr);
    CHECK(application->GetPipelines()->Get(Quasar::PipelineLibrary::Unlit) != nullptr);
    CHECK(application->GetPipelines()->Get(Quasar::PipelineLibrary::BlinnPhong) != nullptr);
    CHECK_FALSE(TestHelpers::HasLogAtLeast(Quasar::LogLevel::Warning));
}

TEST_CASE("The application clears the window with its clear color before the layers render, unless it has none")
{
    const TestHelpers::ScopedLog log;

    CHECK(RenderPixel(glm::vec4(1.0f, 0.0f, 0.0f, 1.0f)) == Color{255, 0, 0, 255});
    CHECK(RenderPixel(std::nullopt) == Color{0, 255, 0, 255});
}
