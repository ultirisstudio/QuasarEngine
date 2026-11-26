#include "helpers/LogHelpers.h"
#include "helpers/NullWindow.h"
#include "helpers/RenderHelpers.h"

#include <quasar/core/Base.h>
#include <quasar/core/Window.h>
#include <quasar/io/EngineResources.h>
#include <quasar/log/MemoryLogSink.h>
#include <quasar/renderer/AmbientOcclusion.h>
#include <quasar/renderer/Camera.h>
#include <quasar/renderer/Framebuffer.h>
#include <quasar/renderer/GraphicsContext.h>
#include <quasar/renderer/Mesh.h>
#include <quasar/renderer/Pipeline.h>
#include <quasar/renderer/Primitives.h>
#include <quasar/renderer/Renderer.h>
#include <quasar/renderer/Shader.h>

#include <doctest/doctest.h>

#include <glad/gl.h>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/mat4x4.hpp>
#include <glm/trigonometric.hpp>
#include <glm/vec2.hpp>
#include <glm/vec3.hpp>
#include <glm/vec4.hpp>

#include <algorithm>
#include <array>
#include <cmath>
#include <cstddef>
#include <optional>

using Quasar::AmbientOcclusionSettings;

namespace
{
    constexpr int Size = 64;

    class Fixture
    {
    public:
        Fixture()
        {
            m_Window = TestHelpers::CreateWindowWithContext();
            REQUIRE(m_Window != nullptr);
            Quasar::GraphicsContext& context = *m_Window->GetGraphicsContext();
            const std::optional<Quasar::ShaderSpecification> depth =
                Quasar::ShaderSpecification::FromFile(Quasar::GetEngineResourcesDirectory() / "shaders" / "ShadowDepth.glsl");
            REQUIRE(depth.has_value());
            m_Shader = context.CreateShader(*depth);
            REQUIRE(m_Shader != nullptr);
            m_Pipeline = context.CreatePipeline({.Shader = m_Shader.get()});
            m_Renderer = Quasar::Renderer::Create(context);
            m_Quad = Quasar::Mesh::Create(context, Quasar::Primitives::CreateQuad());
            m_Cube = Quasar::Mesh::Create(context, Quasar::Primitives::CreateCube());
            m_Depth = context.CreateFramebuffer({.Width = Size, .Height = Size, .Attachments = {Quasar::FramebufferFormat::Depth32F}});
            m_Occlusion = Quasar::AmbientOcclusion::Create(context);
            REQUIRE(m_Pipeline != nullptr);
            REQUIRE(m_Renderer != nullptr);
            REQUIRE(m_Quad != nullptr);
            REQUIRE(m_Cube != nullptr);
            REQUIRE(m_Depth != nullptr);
            REQUIRE(m_Occlusion != nullptr);
            m_Camera.SetViewportSize(Size, Size);
            m_Camera.SetPerspective({.VerticalFov = glm::radians(60.0f), .Near = 0.1f, .Far = 50.0f});
            m_Camera.SetPosition({0.0f, 3.0f, 4.0f});
            m_Camera.LookAt({0.0f, 0.0f, 0.0f});
        }

        Quasar::Camera& GetCamera() { return m_Camera; }
        Quasar::AmbientOcclusion& GetOcclusion() const { return *m_Occlusion; }
        const Quasar::Framebuffer& GetDepth() const { return *m_Depth; }

        void Draw(bool ground, bool cube, bool front = false) const
        {
            m_Depth->ClearDepthStencil();
            m_Renderer->BeginScene(m_Camera, m_Depth.get());
            if (ground)
            {
                const glm::mat4 flat = glm::rotate(glm::mat4(1.0f), glm::radians(-90.0f), glm::vec3(1.0f, 0.0f, 0.0f));
                m_Renderer->Submit(*m_Quad, *m_Pipeline, glm::scale(flat, glm::vec3(20.0f)));
            }
            if (cube)
            {
                m_Renderer->Submit(*m_Cube, *m_Pipeline, glm::translate(glm::mat4(1.0f), {0.0f, 0.5f, 0.0f}));
            }
            if (front)
            {
                m_Renderer->Submit(*m_Quad, *m_Pipeline, glm::scale(glm::translate(glm::mat4(1.0f), {0.0f, 0.93f, 2.0f}), {1.2f, 0.6f, 1.0f}));
            }
            m_Renderer->EndScene();
        }

        float Compute(const AmbientOcclusionSettings& settings, const glm::vec3& point) const
        {
            m_Occlusion->Compute(*m_Depth, m_Camera, settings);
            return Read(point);
        }

        float Read(const glm::vec3& point) const
        {
            const glm::vec4 clip = m_Camera.GetViewProjection() * glm::vec4(point, 1.0f);
            return ReadPixel((glm::vec2(clip) / clip.w * 0.5f + 0.5f) * static_cast<float>(Size));
        }

        float ReadPixel(const glm::vec2& pixel) const
        {
            m_Occlusion->GetResult().BindColorTexture(0, 7);
            glActiveTexture(GL_TEXTURE7);
            GLint texture = 0;
            glGetIntegerv(GL_TEXTURE_BINDING_2D, &texture);
            return static_cast<float>(TestHelpers::ReadRed8(static_cast<GLuint>(texture), static_cast<int>(pixel.x), static_cast<int>(pixel.y))) / 255.0f;
        }

    private:
        Quasar::Scope<Quasar::Window> m_Window;
        Quasar::Scope<Quasar::Shader> m_Shader;
        Quasar::Scope<Quasar::Pipeline> m_Pipeline;
        Quasar::Scope<Quasar::Renderer> m_Renderer;
        Quasar::Scope<Quasar::Mesh> m_Quad;
        Quasar::Scope<Quasar::Mesh> m_Cube;
        Quasar::Scope<Quasar::Framebuffer> m_Depth;
        Quasar::Scope<Quasar::AmbientOcclusion> m_Occlusion;
        Quasar::Camera m_Camera;
    };
}

TEST_CASE("Nothing is darkened where nothing was drawn, nor on an open floor")
{
    const TestHelpers::ScopedLog log;
    Fixture fixture;

    fixture.Draw(false, false);
    CHECK(fixture.Compute({}, {0.0f, 0.0f, 0.0f}) == 1.0f);
    CHECK(fixture.GetOcclusion().GetResult().GetSpecification().Width == Size);
    CHECK(fixture.GetOcclusion().GetResult().GetSpecification().Attachments.front() == Quasar::FramebufferFormat::Red8);

    fixture.Draw(true, false);
    fixture.GetOcclusion().Compute(fixture.GetDepth(), fixture.GetCamera(), {});
    for (const int x : {0, 31, 63})
    {
        CAPTURE(x);
        CHECK(fixture.ReadPixel({x, 0}) > 0.97f);
        CHECK(fixture.ReadPixel({x, 20}) > 0.97f);
    }
    for (const glm::vec3& point : {glm::vec3(0.0f), glm::vec3(1.5f, 0.0f, 1.0f), glm::vec3(-2.0f, 0.0f, -1.0f)})
    {
        CAPTURE(point.x);
        CHECK(fixture.Compute({}, point) > 0.97f);
    }
    CHECK(glGetError() == GL_NO_ERROR);
    CHECK_FALSE(TestHelpers::HasLogAtLeast(Quasar::LogLevel::Warning));
}

TEST_CASE("The floor is darkened where it meets an object, and the top of the object is not")
{
    const TestHelpers::ScopedLog log;
    const Fixture fixture;
    fixture.Draw(true, true);
    const glm::vec3 crease(0.0f, 0.0f, 0.55f);
    const glm::vec3 open(2.0f, 0.0f, 1.5f);

    const float darkened = fixture.Compute({}, crease);
    CAPTURE(darkened);
    CHECK(darkened < 0.85f);
    CHECK(fixture.Read(open) > 0.97f);
    for (const glm::vec3& point :
         {glm::vec3(0.0f, 1.0f, 0.0f), glm::vec3(0.46f, 1.0f, 0.0f), glm::vec3(-0.46f, 1.0f, 0.0f), glm::vec3(0.0f, 1.0f, 0.46f), glm::vec3(0.0f, 0.0f, -1.6f)})
    {
        CAPTURE(point.x);
        CAPTURE(point.z);
        CHECK(fixture.Read(point) > 0.95f);
    }
    for (const float x : {-0.3f, 0.0f, 0.3f})
    {
        for (const float z : {-0.49f, 0.49f})
        {
            CAPTURE(x);
            CAPTURE(z);
            CHECK(fixture.Read({x, 0.999f, z}) > 0.95f);
        }
    }

    CHECK(fixture.Compute({.Intensity = 0.0f}, crease) == 1.0f);
    CHECK(fixture.Compute({.Radius = 0.0f}, crease) == 1.0f);
    CHECK(fixture.Compute({.Radius = -1.0f}, crease) == 1.0f);
    CHECK(fixture.Compute({.Intensity = 3.0f}, crease) < darkened - 0.05f);
    CHECK(fixture.Compute({.Radius = 2.0f}, {0.0f, 0.0f, 1.2f}) < fixture.Compute({.Radius = 0.3f}, {0.0f, 0.0f, 1.2f}) - 0.05f);
    CHECK_FALSE(TestHelpers::HasLogAtLeast(Quasar::LogLevel::Warning));
}

TEST_CASE("The ambient occlusion works with an orthographic camera and with any number of samples")
{
    const TestHelpers::ScopedLog log;
    Fixture fixture;
    const glm::vec3 crease(0.0f, 0.0f, 0.55f);

    fixture.Draw(true, true);
    const float few = fixture.Compute({.Samples = 0}, crease);
    CHECK(few == fixture.Compute({.Samples = 1}, crease));
    CHECK(fixture.Compute({.Samples = 1000}, crease) < 0.85f);

    fixture.GetCamera().SetOrthographic({.Height = 6.0f, .Near = 0.1f, .Far = 50.0f});
    fixture.Draw(true, true);
    CHECK(fixture.Compute({}, crease) < 0.85f);
    CHECK(fixture.Read({2.0f, 0.0f, 1.5f}) > 0.97f);
    CHECK(glGetError() == GL_NO_ERROR);
    CHECK_FALSE(TestHelpers::HasLogAtLeast(Quasar::LogLevel::Warning));
}

TEST_CASE("The ambient occlusion at half resolution is close to the full one, and keeps the edges of the objects")
{
    const TestHelpers::ScopedLog log;
    Fixture fixture;
    fixture.Draw(true, true);
    const std::array<glm::vec3, 5> points{glm::vec3(0.0f, 0.0f, 0.55f), glm::vec3(0.55f, 0.0f, 0.3f), glm::vec3(2.0f, 0.0f, 1.5f), glm::vec3(0.0f, 1.0f, 0.0f),
                                          glm::vec3(0.0f, 0.999f, 0.49f)};

    std::array<float, points.size()> full{};
    fixture.GetOcclusion().Compute(fixture.GetDepth(), fixture.GetCamera(), {.HalfResolution = false});
    for (std::size_t index = 0; index < points.size(); ++index)
    {
        full.at(index) = fixture.Read(points.at(index));
    }
    fixture.GetOcclusion().Compute(fixture.GetDepth(), fixture.GetCamera(), {.HalfResolution = true});

    CHECK(fixture.GetOcclusion().GetResult().GetSpecification().Width == Size);
    CHECK(full.front() < 0.85f);
    for (std::size_t index = 0; index < points.size(); ++index)
    {
        CAPTURE(index);
        CAPTURE(full.at(index));
        CHECK(std::abs(fixture.Read(points.at(index)) - full.at(index)) < 0.1f);
    }
    CHECK(glGetError() == GL_NO_ERROR);
    CHECK_FALSE(TestHelpers::HasLogAtLeast(Quasar::LogLevel::Warning));
}

TEST_CASE("At half resolution, the darkening of a crease does not spread to an object in front of it")
{
    const TestHelpers::ScopedLog log;
    Fixture fixture;
    fixture.Draw(true, true, true);

    for (const bool half : {false, true})
    {
        CAPTURE(half);
        fixture.GetOcclusion().Compute(fixture.GetDepth(), fixture.GetCamera(), {.HalfResolution = half});
        float darkest = 1.0f;
        for (int row = 0; row <= 56; ++row)
        {
            for (int column = 0; column <= 10; ++column)
            {
                darkest = std::min(darkest, fixture.Read({(static_cast<float>(column) * 0.1f) - 0.5f, 0.65f + (static_cast<float>(row) * 0.01f), 2.0f}));
            }
        }
        CHECK(darkest > 0.97f);
    }
    fixture.Draw(true, true);
    CHECK(fixture.Compute({}, {0.0f, 0.0f, 0.55f}) < 0.85f);
    CHECK_FALSE(TestHelpers::HasLogAtLeast(Quasar::LogLevel::Warning));
}
