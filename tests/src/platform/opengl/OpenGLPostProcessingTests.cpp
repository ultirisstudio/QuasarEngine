#include "helpers/LogHelpers.h"
#include "helpers/NullWindow.h"
#include "helpers/RenderHelpers.h"

#include <quasar/core/Base.h>
#include <quasar/core/Window.h>
#include <quasar/log/MemoryLogSink.h>
#include <quasar/renderer/Camera.h>
#include <quasar/renderer/Framebuffer.h>
#include <quasar/renderer/FullscreenPass.h>
#include <quasar/renderer/GraphicsContext.h>
#include <quasar/renderer/Pipeline.h>
#include <quasar/renderer/PostProcessing.h>
#include <quasar/renderer/Shader.h>
#include <quasar/renderer/ToneMapper.h>

#include <doctest/doctest.h>

#include <glad/gl.h>
#include <glm/vec2.hpp>
#include <glm/vec4.hpp>

#include <cstdlib>
#include <optional>
#include <string>
#include <string_view>
#include <utility>

using Quasar::PostProcessStage;
using TestHelpers::Color;
using TestHelpers::IsNear;
using TestHelpers::ReadColor;
using TestHelpers::ReadWindowPixel;

namespace
{
    bool IsNear(int value, int expected)
    {
        return std::abs(value - expected) <= 1;
    }

    constexpr std::string_view LinearShader = R"(
#type vertex
#version 450 core
void main()
{
    const vec2 positions[3] = vec2[](vec2(-1.0, -1.0), vec2(3.0, -1.0), vec2(-1.0, 3.0));
    gl_Position = vec4(positions[gl_VertexID], 0.0, 1.0);
}

#type fragment
#version 450 core
layout(binding = 0) uniform sampler2D u_Source;
uniform float u_Scale;
uniform float u_Offset;
layout(location = 0) out vec4 o_Color;
void main()
{
    vec4 color = texelFetch(u_Source, ivec2(gl_FragCoord.xy), 0);
    o_Color = vec4(color.rgb * u_Scale + u_Offset, 1.0);
}
)";

    class LinearEffect final : public Quasar::PostProcessEffect
    {
    public:
        LinearEffect(Quasar::GraphicsContext& context, std::string name, PostProcessStage stage, float scale, float offset)
            : PostProcessEffect(std::move(name), stage), m_Scale(scale), m_Offset(offset)
        {
            const std::optional<Quasar::ShaderSpecification> specification = Quasar::ShaderSpecification::Parse("Linear", LinearShader);
            REQUIRE(specification.has_value());
            m_Shader = context.CreateShader(*specification);
            REQUIRE(m_Shader != nullptr);
            m_Pipeline = context.CreatePipeline({.Shader = m_Shader.get(), .DepthTest = false, .DepthWrite = false, .Culling = Quasar::CullMode::None});
            m_Pass = Quasar::FullscreenPass::Create(context);
            REQUIRE(m_Pipeline != nullptr);
            REQUIRE(m_Pass != nullptr);
        }

        void Apply(const Quasar::PostProcessInput& input, Quasar::Framebuffer* target) override
        {
            m_LastInput = input;
            m_LastTarget = target;
            ++m_Calls;
            m_Shader->SetFloat("u_Scale", m_Scale);
            m_Shader->SetFloat("u_Offset", m_Offset);
            input.Source->BindColorTexture(0, 0);
            m_Pass->Draw(*m_Pipeline, target);
        }

        const Quasar::PostProcessInput& GetLastInput() const { return m_LastInput; }
        Quasar::Framebuffer* GetLastTarget() const { return m_LastTarget; }
        int GetCalls() const { return m_Calls; }

    private:
        Quasar::Scope<Quasar::Shader> m_Shader;
        Quasar::Scope<Quasar::Pipeline> m_Pipeline;
        Quasar::Scope<Quasar::FullscreenPass> m_Pass;
        float m_Scale;
        float m_Offset;
        Quasar::PostProcessInput m_LastInput{};
        Quasar::Framebuffer* m_LastTarget = nullptr;
        int m_Calls = 0;
    };

    class OtherEffect final : public Quasar::PostProcessEffect
    {
    public:
        OtherEffect() : PostProcessEffect("Other", PostProcessStage::AfterToneMapping) {}

        void Apply(const Quasar::PostProcessInput& /*input*/, Quasar::Framebuffer* /*target*/) override {}
    };

    class Fixture
    {
    public:
        Fixture()
        {
            m_Window = TestHelpers::CreateWindowWithContext();
            REQUIRE(m_Window != nullptr);
            m_Stack = Quasar::PostProcessStack::Create(GetContext());
            m_Scene = GetContext().CreateFramebuffer(
                {.Width = 4, .Height = 4, .Attachments = {Quasar::FramebufferFormat::Rgba16F, Quasar::FramebufferFormat::Depth24Stencil8}});
            m_Target = GetContext().CreateFramebuffer({.Width = 4, .Height = 4});
            REQUIRE(m_Stack != nullptr);
            REQUIRE(m_Scene != nullptr);
            REQUIRE(m_Target != nullptr);
        }

        Quasar::GraphicsContext& GetContext() const { return *m_Window->GetGraphicsContext(); }
        Quasar::PostProcessStack& GetStack() const { return *m_Stack; }
        Quasar::Framebuffer& GetScene() const { return *m_Scene; }
        Quasar::Framebuffer& GetTarget() const { return *m_Target; }
        const Quasar::Camera& GetCamera() const { return m_Camera; }

        LinearEffect* Add(std::string name, PostProcessStage stage, float scale, float offset) const
        {
            return m_Stack->Add(Quasar::CreateScope<LinearEffect>(GetContext(), std::move(name), stage, scale, offset));
        }

        int Render(float linear) const
        {
            m_Scene->ClearColor(0, {linear, linear, linear, 1.0f});
            m_Stack->Apply(*m_Scene, m_Target.get(), m_Camera, 1.0f, Quasar::Tonemapping::None);
            return ReadColor(*m_Target, 1, 1)[0];
        }

    private:
        Quasar::Scope<Quasar::Window> m_Window;
        Quasar::Scope<Quasar::PostProcessStack> m_Stack;
        Quasar::Scope<Quasar::Framebuffer> m_Scene;
        Quasar::Scope<Quasar::Framebuffer> m_Target;
        Quasar::Camera m_Camera;
    };
}

TEST_CASE("Without effects, a post-processing stack only applies the tone mapping")
{
    const TestHelpers::ScopedLog log;
    const Fixture fixture;

    CHECK(fixture.GetStack().GetEffects().empty());
    CHECK(IsNear(fixture.Render(0.5f), 188));
    fixture.GetScene().ClearColor(0, {2.0f, 2.0f, 2.0f, 1.0f});
    fixture.GetStack().Apply(fixture.GetScene(), &fixture.GetTarget(), fixture.GetCamera(), 0.5f, Quasar::Tonemapping::Reinhard);
    CHECK(IsNear(ReadColor(fixture.GetTarget(), 1, 1), {188, 188, 188, 255}));
    CHECK_FALSE(TestHelpers::HasLogAtLeast(Quasar::LogLevel::Warning));
}

TEST_CASE("The effects before the tone mapping run in their order on linear colors that can go above 1")
{
    const TestHelpers::ScopedLog log;
    const Fixture fixture;
    LinearEffect* first = fixture.Add("Double", PostProcessStage::BeforeToneMapping, 4.0f, 0.0f);
    LinearEffect* second = fixture.Add("Quarter", PostProcessStage::BeforeToneMapping, 0.25f, 0.1f);
    REQUIRE(first != nullptr);
    REQUIRE(second != nullptr);

    CHECK(IsNear(fixture.Render(0.5f), 203));
    CHECK(first->GetLastInput().Source == &fixture.GetScene());
    CHECK(first->GetLastInput().Scene == &fixture.GetScene());
    CHECK(first->GetLastInput().View == &fixture.GetCamera());
    CHECK(first->GetLastInput().Size == glm::uvec2(4, 4));
    CHECK(second->GetLastInput().Source == first->GetLastTarget());
    CHECK(second->GetLastInput().Scene == &fixture.GetScene());
    REQUIRE(second->GetLastTarget() != nullptr);
    CHECK(second->GetLastTarget() != first->GetLastTarget());
    CHECK(first->GetLastTarget() != &fixture.GetScene());
    CHECK(second->GetLastTarget() != &fixture.GetTarget());
    CHECK(second->GetLastTarget()->GetSpecification().Attachments.front() == Quasar::FramebufferFormat::Rgba16F);
    CHECK(second->GetLastTarget()->GetSpecification().Width == 4);

    LinearEffect* third = fixture.Add("Half", PostProcessStage::BeforeToneMapping, 0.5f, 0.0f);
    REQUIRE(third != nullptr);
    CHECK(IsNear(fixture.Render(0.5f), 149));
    CHECK(third->GetLastInput().Source == second->GetLastTarget());
    CHECK(third->GetLastTarget() == first->GetLastTarget());
    CHECK_FALSE(TestHelpers::HasLogAtLeast(Quasar::LogLevel::Warning));
}

TEST_CASE("The effects after the tone mapping run on the sRGB colors of the screen, and the last one writes the target")
{
    const TestHelpers::ScopedLog log;
    const Fixture fixture;
    LinearEffect* brighten = fixture.Add("Brighten", PostProcessStage::AfterToneMapping, 1.0f, 0.1f);
    LinearEffect* linear = fixture.Add("Linear", PostProcessStage::BeforeToneMapping, 2.0f, 0.0f);
    REQUIRE(brighten != nullptr);
    REQUIRE(linear != nullptr);

    CHECK(IsNear(fixture.Render(0.25f), 214));
    CHECK(brighten->GetLastTarget() == &fixture.GetTarget());
    REQUIRE(brighten->GetLastInput().Source != nullptr);
    CHECK(brighten->GetLastInput().Source->GetSpecification().Attachments.front() == Quasar::FramebufferFormat::Rgba8);
    CHECK(brighten->GetLastInput().Source->GetSpecification().Width == 4);

    LinearEffect* darken = fixture.Add("Darken", PostProcessStage::AfterToneMapping, 0.5f, 0.0f);
    REQUIRE(darken != nullptr);
    CHECK(IsNear(fixture.Render(0.25f), 107));
    CHECK(brighten->GetLastTarget() != &fixture.GetTarget());
    CHECK(brighten->GetLastTarget() != brighten->GetLastInput().Source);
    CHECK(darken->GetLastInput().Source == brighten->GetLastTarget());
    CHECK(darken->GetLastTarget() == &fixture.GetTarget());

    fixture.GetContext().ClearWindow({0.0f, 0.0f, 1.0f, 1.0f});
    fixture.GetScene().ClearColor(0, {0.25f, 0.25f, 0.25f, 1.0f});
    fixture.GetStack().Apply(fixture.GetScene(), nullptr, fixture.GetCamera(), 1.0f, Quasar::Tonemapping::None);
    CHECK(darken->GetLastTarget() == nullptr);
    CHECK(IsNear(ReadWindowPixel(1, 1), {107, 107, 107, 255}));
    GLint framebuffer = -1;
    glGetIntegerv(GL_DRAW_FRAMEBUFFER_BINDING, &framebuffer);
    CHECK(framebuffer == 0);
    CHECK_FALSE(TestHelpers::HasLogAtLeast(Quasar::LogLevel::Warning));
}

TEST_CASE("A disabled effect is skipped, and an effect can be found by its type and removed")
{
    const TestHelpers::ScopedLog log;
    const Fixture fixture;
    LinearEffect* doubled = fixture.Add("Double", PostProcessStage::BeforeToneMapping, 2.0f, 0.0f);
    REQUIRE(doubled != nullptr);
    CHECK(doubled->GetName() == "Double");
    CHECK(doubled->GetStage() == PostProcessStage::BeforeToneMapping);
    CHECK(doubled->IsEnabled());

    doubled->SetEnabled(false);
    CHECK_FALSE(doubled->IsEnabled());
    CHECK(IsNear(fixture.Render(0.25f), 137));
    CHECK(doubled->GetCalls() == 0);
    doubled->SetEnabled(true);
    CHECK(IsNear(fixture.Render(0.25f), 188));
    CHECK(doubled->GetCalls() == 1);

    CHECK(fixture.GetStack().Find<LinearEffect>() == doubled);
    CHECK(fixture.GetStack().Find<OtherEffect>() == nullptr);
    OtherEffect* other = fixture.GetStack().Add(Quasar::CreateScope<OtherEffect>());
    REQUIRE(other != nullptr);
    CHECK(fixture.GetStack().Find<OtherEffect>() == other);
    CHECK(fixture.GetStack().Find<Quasar::PostProcessEffect>() == doubled);
    REQUIRE(fixture.GetStack().GetEffects().size() == 2);
    CHECK(fixture.GetStack().GetEffects()[1].get() == other);

    CHECK(fixture.GetStack().Remove(*doubled));
    CHECK(fixture.GetStack().GetEffects().size() == 1);
    CHECK(fixture.GetStack().Find<LinearEffect>() == nullptr);
    const OtherEffect outside;
    CHECK_FALSE(fixture.GetStack().Remove(outside));
    CHECK(fixture.GetStack().GetEffects().size() == 1);
    CHECK_FALSE(TestHelpers::HasLogAtLeast(Quasar::LogLevel::Warning));
}

TEST_CASE("A post-processing stack refuses an effect that does not exist")
{
    const TestHelpers::ScopedLog log;
    const Fixture fixture;

    CHECK(fixture.GetStack().Add(Quasar::Scope<LinearEffect>()) == nullptr);
    CHECK(fixture.GetStack().GetEffects().empty());
    CHECK(TestHelpers::CountErrors("does not exist") == 1);
}
