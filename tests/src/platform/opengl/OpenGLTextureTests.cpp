#include "helpers/LogHelpers.h"
#include "helpers/NullWindow.h"
#include "helpers/TestLayers.h"

#include <quasar/core/Application.h>
#include <quasar/core/Base.h>
#include <quasar/core/Layer.h>
#include <quasar/core/Window.h>
#include <quasar/log/MemoryLogSink.h>
#include <quasar/renderer/Framebuffer.h>
#include <quasar/renderer/GraphicsContext.h>
#include <quasar/renderer/Shader.h>
#include <quasar/renderer/Texture.h>
#include <quasar/renderer/VertexArray.h>

#include <doctest/doctest.h>

#include <glad/gl.h>
#include <glm/vec2.hpp>
#include <imgui.h>

#include <array>
#include <cstddef>
#include <cstdint>
#include <cstdlib>
#include <optional>
#include <span>
#include <string_view>
#include <vector>

using Quasar::TextureFilter;
using Quasar::TextureFormat;
using Quasar::TextureSpecification;
using Quasar::TextureWrap;

namespace
{
    // Each pixel of the framebuffer reads the pixel of the texture at the same row and column, counted from the top, and
    // writes one channel of it, multiplied by u_Scale
    constexpr std::string_view SamplingShader = R"(
#type vertex
#version 450 core

void main()
{
    const vec2 positions[3] = vec2[](vec2(-1.0, -1.0), vec2(3.0, -1.0), vec2(-1.0, 3.0));
    gl_Position = vec4(positions[gl_VertexID], 0.0, 1.0);
}

#type fragment
#version 450 core
layout(binding = 1) uniform sampler2D u_Texture;
uniform vec2 u_Size;
uniform vec2 u_Offset;
uniform float u_Lod;
uniform int u_Channel;
uniform float u_Scale;
layout(location = 0) out int o_Value;

void main()
{
    const vec2 coordinates = vec2(gl_FragCoord.x, u_Size.y - gl_FragCoord.y) / u_Size + u_Offset;
    o_Value = int(round(textureLod(u_Texture, coordinates, u_Lod)[u_Channel] * u_Scale));
}
)";

    struct Sampling
    {
        glm::vec2 Offset{0.0f};
        float Lod = 0.0f;
        int Channel = 0;
        float Scale = 255.0f;
    };

    class Scene
    {
    public:
        explicit Scene(std::uint32_t width = 4, std::uint32_t height = 4) : m_Window(TestHelpers::CreateWindowWithContext()), m_Width(width), m_Height(height)
        {
            REQUIRE(m_Window != nullptr);
            const std::optional<Quasar::ShaderSpecification> specification = Quasar::ShaderSpecification::Parse("Sampling", SamplingShader);
            REQUIRE(specification.has_value());
            m_Shader = GetContext().CreateShader(*specification);
            m_Framebuffer = GetContext().CreateFramebuffer({.Width = width, .Height = height, .Attachments = {Quasar::FramebufferFormat::RedInteger}});
            m_VertexArray = GetContext().CreateVertexArray();
            REQUIRE(m_Shader != nullptr);
            REQUIRE(m_Framebuffer != nullptr);
            REQUIRE(m_VertexArray != nullptr);
        }

        Quasar::GraphicsContext& GetContext() const { return *m_Window->GetGraphicsContext(); }

        Quasar::Scope<Quasar::Texture2D> CreateTexture(const TextureSpecification& specification, std::span<const std::byte> pixels) const
        {
            Quasar::Scope<Quasar::Texture2D> texture = GetContext().CreateTexture2D(specification, pixels);
            REQUIRE(texture != nullptr);
            return texture;
        }

        std::vector<int> Draw(const Sampling& sampling = {}) const
        {
            m_Shader->SetVec2("u_Size", {static_cast<float>(m_Width), static_cast<float>(m_Height)});
            m_Shader->SetVec2("u_Offset", sampling.Offset);
            m_Shader->SetFloat("u_Lod", sampling.Lod);
            m_Shader->SetInt("u_Channel", sampling.Channel);
            m_Shader->SetFloat("u_Scale", sampling.Scale);
            m_Framebuffer->ClearInteger(0, -1);
            m_Framebuffer->Bind();
            m_Shader->Bind();
            GetContext().Draw(*m_VertexArray, 3);
            m_Framebuffer->Unbind();

            std::vector<int> values;
            for (std::uint32_t y = 0; y < m_Height; ++y)
            {
                for (std::uint32_t x = 0; x < m_Width; ++x)
                {
                    values.push_back(m_Framebuffer->ReadInteger(0, x, y).value_or(-2));
                }
            }
            return values;
        }

        int DrawFirst(const Sampling& sampling) const { return Draw(sampling).front(); }

    private:
        Quasar::Scope<Quasar::Window> m_Window;
        std::uint32_t m_Width;
        std::uint32_t m_Height;
        Quasar::Scope<Quasar::Shader> m_Shader;
        Quasar::Scope<Quasar::Framebuffer> m_Framebuffer;
        Quasar::Scope<Quasar::VertexArray> m_VertexArray;
    };

    template <typename T>
    std::span<const std::byte> AsBytes(const std::vector<T>& values)
    {
        return std::as_bytes(std::span(values));
    }

    // 4x4 pixels of Rgba8 whose red is the column plus 10 times the row
    std::vector<std::uint8_t> MakeGrid()
    {
        std::vector<std::uint8_t> pixels;
        for (std::uint8_t y = 0; y < 4; ++y)
        {
            for (std::uint8_t x = 0; x < 4; ++x)
            {
                const auto red = static_cast<std::uint8_t>(x + 10 * y);
                pixels.insert(pixels.end(), {red, static_cast<std::uint8_t>(100 + red), static_cast<std::uint8_t>(200 - red), 255});
            }
        }
        return pixels;
    }

    std::vector<int> Channel(const std::vector<std::uint8_t>& pixels, std::size_t channel, std::size_t channelCount)
    {
        std::vector<int> values;
        for (std::size_t index = channel; index < pixels.size(); index += channelCount)
        {
            values.push_back(pixels[index]);
        }
        return values;
    }

    bool IsNear(int value, int expected)
    {
        return std::abs(value - expected) <= 1;
    }

    class ImageLayer : public Quasar::Layer
    {
    public:
        explicit ImageLayer(const Quasar::Texture2D& texture) : Layer("Image"), m_Texture(&texture) {}

        void OnImGuiRender() override
        {
            ImGui::Begin("Texture");
            ImGui::Image(m_Texture->GetImGuiTexture(), ImVec2(16.0f, 16.0f));
            ImGui::End();
        }

    private:
        const Quasar::Texture2D* m_Texture;
    };
}

TEST_CASE("A texture is read from its top row at the texture coordinate v = 0, with its channels in order")
{
    const TestHelpers::ScopedLog log;
    const Scene scene;
    const std::vector<std::uint8_t> grid = MakeGrid();
    const Quasar::Scope<Quasar::Texture2D> texture =
        scene.CreateTexture({.Width = 4, .Height = 4, .Filter = TextureFilter::Nearest, .Mipmaps = false}, AsBytes(grid));
    texture->Bind(1);

    for (int channel = 0; channel < 4; ++channel)
    {
        CAPTURE(channel);
        CHECK(scene.Draw({.Channel = channel}) == Channel(grid, static_cast<std::size_t>(channel), 4));
    }
    CHECK(glGetError() == GL_NO_ERROR);
    CHECK_FALSE(TestHelpers::HasLogAtLeast(Quasar::LogLevel::Warning));
}

TEST_CASE("An R8 texture is read in its red channel, whatever the width of its rows")
{
    const TestHelpers::ScopedLog log;
    const Scene scene(3, 2);
    const std::vector<std::uint8_t> pixels{1, 2, 3, 4, 5, 6};
    GLint alignment = 0;
    glGetIntegerv(GL_UNPACK_ALIGNMENT, &alignment);

    const Quasar::Scope<Quasar::Texture2D> texture =
        scene.CreateTexture({.Width = 3, .Height = 2, .Format = TextureFormat::R8, .Filter = TextureFilter::Nearest}, AsBytes(pixels));
    texture->Bind(1);

    CHECK(scene.Draw() == std::vector<int>{1, 2, 3, 4, 5, 6});
    CHECK(scene.Draw({.Channel = 1}) == std::vector<int>(6, 0));
    CHECK(scene.Draw({.Channel = 3}) == std::vector<int>(6, 255));
    GLint restored = 0;
    glGetIntegerv(GL_UNPACK_ALIGNMENT, &restored);
    CHECK(restored == alignment);
    CHECK(glGetError() == GL_NO_ERROR);
}

TEST_CASE("An Srgba8 texture gives linear colors, and an Rgba16F texture keeps values out of 0 to 1")
{
    const TestHelpers::ScopedLog log;
    const Scene scene(1, 1);
    const std::vector<std::uint8_t> color{188, 188, 188, 188};
    const std::vector<float> hdr{2.5f, -1.0f, 1000.0f, 0.25f};
    const Quasar::Scope<Quasar::Texture2D> linear = scene.CreateTexture({.Format = TextureFormat::Rgba8}, AsBytes(color));
    const Quasar::Scope<Quasar::Texture2D> srgb = scene.CreateTexture({.Format = TextureFormat::Srgba8}, AsBytes(color));
    const Quasar::Scope<Quasar::Texture2D> floats = scene.CreateTexture({.Format = TextureFormat::Rgba16F}, AsBytes(hdr));

    linear->Bind(1);
    CHECK(IsNear(scene.DrawFirst({.Scale = 1000.0f}), 737));
    srgb->Bind(1);
    CHECK(IsNear(scene.DrawFirst({.Scale = 1000.0f}), 503));
    CHECK(IsNear(scene.DrawFirst({.Channel = 3, .Scale = 1000.0f}), 737));
    floats->Bind(1);
    CHECK(scene.DrawFirst({.Channel = 0, .Scale = 4.0f}) == 10);
    CHECK(scene.DrawFirst({.Channel = 1, .Scale = 4.0f}) == -4);
    CHECK(scene.DrawFirst({.Channel = 2, .Scale = 4.0f}) == 4000);
    CHECK(scene.DrawFirst({.Channel = 3, .Scale = 4.0f}) == 1);
    CHECK(glGetError() == GL_NO_ERROR);
    CHECK_FALSE(TestHelpers::HasLogAtLeast(Quasar::LogLevel::Warning));
}

TEST_CASE("A Linear texture blends the closest pixels, and a Nearest texture reads the closest one")
{
    const TestHelpers::ScopedLog log;
    const Scene scene(4, 1);
    const std::vector<std::uint8_t> pixels{0, 100, 200, 250};
    const Quasar::Scope<Quasar::Texture2D> nearest =
        scene.CreateTexture({.Width = 4, .Format = TextureFormat::R8, .Filter = TextureFilter::Nearest, .Wrap = TextureWrap::ClampToEdge}, AsBytes(pixels));
    const Quasar::Scope<Quasar::Texture2D> linear =
        scene.CreateTexture({.Width = 4, .Format = TextureFormat::R8, .Filter = TextureFilter::Linear, .Wrap = TextureWrap::ClampToEdge}, AsBytes(pixels));
    const Sampling between{.Offset = {0.05f, 0.0f}};

    nearest->Bind(1);
    CHECK(scene.Draw(between) == std::vector<int>{0, 100, 200, 250});
    linear->Bind(1);
    const std::vector<int> blended = scene.Draw(between);
    REQUIRE(blended.size() == 4);
    CHECK(IsNear(blended[0], 20));
    CHECK(IsNear(blended[1], 120));
    CHECK(IsNear(blended[2], 210));
    CHECK(blended[3] == 250);
}

TEST_CASE("A texture is repeated, mirrored or clamped outside of the texture coordinates 0 to 1")
{
    const TestHelpers::ScopedLog log;
    const Scene scene;
    const std::vector<std::uint8_t> grid = MakeGrid();
    const Sampling outside{.Offset = {1.25f, 1.25f}};

    struct Expected
    {
        TextureWrap Wrap = TextureWrap::Repeat;
        int Value = 0;
    };
    for (const Expected expected : std::array<Expected, 3>{{{TextureWrap::Repeat, 11}, {TextureWrap::MirroredRepeat, 22}, {TextureWrap::ClampToEdge, 33}}})
    {
        CAPTURE(expected.Value);
        const Quasar::Scope<Quasar::Texture2D> texture =
            scene.CreateTexture({.Width = 4, .Height = 4, .Filter = TextureFilter::Nearest, .Wrap = expected.Wrap, .Mipmaps = false}, AsBytes(grid));
        texture->Bind(1);
        CHECK(scene.DrawFirst(outside) == expected.Value);
    }
}

TEST_CASE("The mipmaps of a texture are made from its pixels, and SetData updates them")
{
    const TestHelpers::ScopedLog log;
    const Scene scene;
    std::vector<std::uint8_t> pixels(16, 0);
    for (std::size_t row = 0; row < 4; ++row)
    {
        pixels[row * 4 + 2] = 200;
        pixels[row * 4 + 3] = 200;
    }
    const TextureSpecification specification{.Width = 4, .Height = 4, .Format = TextureFormat::R8};
    const Quasar::Scope<Quasar::Texture2D> mipmapped = scene.CreateTexture(specification, AsBytes(pixels));
    TextureSpecification withoutMipmaps = specification;
    withoutMipmaps.Mipmaps = false;
    const Quasar::Scope<Quasar::Texture2D> single = scene.CreateTexture(withoutMipmaps, AsBytes(pixels));

    TextureSpecification nearestSpecification = specification;
    nearestSpecification.Filter = TextureFilter::Nearest;
    const Quasar::Scope<Quasar::Texture2D> nearest = scene.CreateTexture(nearestSpecification, AsBytes(pixels));
    const Sampling smallest{.Lod = 2.0f};

    mipmapped->Bind(1);
    CHECK(IsNear(scene.DrawFirst(smallest), 100));
    single->Bind(1);
    CHECK(scene.DrawFirst(smallest) == 0);
    nearest->Bind(1);
    CHECK(scene.Draw({.Lod = 1.0f}) == Channel(pixels, 0, 1));

    mipmapped->SetData(AsBytes(std::vector<std::uint8_t>(16, 50)));
    mipmapped->Bind(1);
    CHECK(scene.DrawFirst(smallest) == 50);
    CHECK(scene.DrawFirst({}) == 50);
    CHECK(glGetError() == GL_NO_ERROR);
    CHECK_FALSE(TestHelpers::HasLogAtLeast(Quasar::LogLevel::Warning));
}

TEST_CASE("A texture created without pixels is filled with zeros, and pixels of another size are refused")
{
    const TestHelpers::ScopedLog log;
    const Scene scene;
    const Quasar::Scope<Quasar::Texture2D> texture = scene.CreateTexture({.Width = 4, .Height = 4}, {});
    texture->Bind(1);
    CHECK(texture->GetDataSize() == 64);

    for (int channel = 0; channel < 4; ++channel)
    {
        CHECK(scene.Draw({.Channel = channel}) == std::vector<int>(16, 0));
    }
    CHECK(scene.DrawFirst({.Lod = 2.0f}) == 0);
    CHECK_FALSE(TestHelpers::HasLogAtLeast(Quasar::LogLevel::Error));

    texture->SetData(AsBytes(std::vector<std::uint8_t>(63, 255)));
    texture->SetData(AsBytes(std::vector<std::uint8_t>(65, 255)));
    CHECK(scene.Draw() == std::vector<int>(16, 0));
    CHECK(TestHelpers::HasLogAtLeast(Quasar::LogLevel::Error));
    CHECK(glGetError() == GL_NO_ERROR);
}

TEST_CASE("Bind makes the draws read a texture from the slot of the sampler")
{
    const TestHelpers::ScopedLog log;
    const Scene scene(1, 1);
    const std::vector<std::uint8_t> first{10, 0, 0, 255};
    const std::vector<std::uint8_t> second{20, 0, 0, 255};
    const Quasar::Scope<Quasar::Texture2D> textureA = scene.CreateTexture({}, AsBytes(first));
    const Quasar::Scope<Quasar::Texture2D> textureB = scene.CreateTexture({}, AsBytes(second));

    textureA->Bind(1);
    CHECK(scene.DrawFirst({}) == 10);
    textureB->Bind(1);
    CHECK(scene.DrawFirst({}) == 20);
    textureA->Bind(0);
    CHECK(scene.DrawFirst({}) == 20);
    CHECK_FALSE(TestHelpers::HasLogAtLeast(Quasar::LogLevel::Error));

    GLint slots = 0;
    glGetIntegerv(GL_MAX_COMBINED_TEXTURE_IMAGE_UNITS, &slots);
    textureA->Bind(static_cast<std::uint32_t>(slots));
    CHECK(scene.DrawFirst({}) == 20);
    CHECK(TestHelpers::HasLogAtLeast(Quasar::LogLevel::Error));
    CHECK(glGetError() == GL_NO_ERROR);
}

TEST_CASE("No texture is created from an empty size, a size larger than OpenGL allows or pixels of another size")
{
    const TestHelpers::ScopedLog log;
    const Quasar::Scope<Quasar::Window> window = TestHelpers::CreateWindowWithContext();
    REQUIRE(window != nullptr);
    Quasar::GraphicsContext& context = *window->GetGraphicsContext();
    GLint maxSize = 0;
    glGetIntegerv(GL_MAX_TEXTURE_SIZE, &maxSize);
    const std::vector<std::uint8_t> pixels(16, 255);

    CHECK(context.CreateTexture2D({.Width = 2, .Height = 2}, AsBytes(pixels)) != nullptr);
    CHECK_FALSE(TestHelpers::HasLogAtLeast(Quasar::LogLevel::Error));

    CHECK(context.CreateTexture2D({.Width = 0, .Height = 2}, {}) == nullptr);
    CHECK(context.CreateTexture2D({.Width = 2, .Height = 0}, {}) == nullptr);
    CHECK(context.CreateTexture2D({.Width = static_cast<std::uint32_t>(maxSize) + 1, .Height = 1}, {}) == nullptr);
    CHECK(context.CreateTexture2D({.Width = 1, .Height = static_cast<std::uint32_t>(maxSize) + 1}, {}) == nullptr);
    CHECK(context.CreateTexture2D({.Width = 3, .Height = 2}, AsBytes(pixels)) == nullptr);
    CHECK(context.CreateTexture2D({.Width = 1, .Height = 1}, AsBytes(pixels)) == nullptr);
    CHECK(context.CreateTexture2D({.Width = 2, .Height = 2, .Format = TextureFormat::Rgba16F}, AsBytes(pixels)) == nullptr);
    CHECK(TestHelpers::HasLogAtLeast(Quasar::LogLevel::Error));
    CHECK(glGetError() == GL_NO_ERROR);
}

TEST_CASE("ImGui draws a texture")
{
    const TestHelpers::ScopedLog log;
    const Quasar::Scope<Quasar::Application> application = TestHelpers::CreateApplicationWithContext();
    REQUIRE(application != nullptr);
    ImGui::GetIO().IniFilename = nullptr;
    const std::vector<std::uint8_t> pixels(16, 255);
    const Quasar::Scope<Quasar::Texture2D> texture = application->GetWindow().GetGraphicsContext()->CreateTexture2D({.Width = 2, .Height = 2}, AsBytes(pixels));
    REQUIRE(texture != nullptr);
    CHECK(texture->GetImGuiTexture() != ImTextureID{});
    application->PushLayer(Quasar::CreateScope<ImageLayer>(*texture));
    application->PushLayer(Quasar::CreateScope<TestHelpers::ClosingLayer>(2));

    application->Run();

    CHECK(glGetError() == GL_NO_ERROR);
    CHECK_FALSE(TestHelpers::HasLogAtLeast(Quasar::LogLevel::Error));
}
