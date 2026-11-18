#include "helpers/LogHelpers.h"
#include "helpers/NullWindow.h"

#include <quasar/core/Base.h>
#include <quasar/core/Window.h>
#include <quasar/log/MemoryLogSink.h>
#include <quasar/renderer/Camera.h>
#include <quasar/renderer/Framebuffer.h>
#include <quasar/renderer/GraphicsContext.h>
#include <quasar/renderer/Mesh.h>
#include <quasar/renderer/Pipeline.h>
#include <quasar/renderer/Primitives.h>
#include <quasar/renderer/Shader.h>

#include <doctest/doctest.h>

#include <glad/gl.h>
#include <glm/gtc/quaternion.hpp>
#include <glm/mat4x4.hpp>
#include <glm/trigonometric.hpp>
#include <glm/vec2.hpp>

#include <array>
#include <cstdint>
#include <optional>
#include <span>
#include <string_view>
#include <vector>

using Quasar::CullMode;

namespace
{
    constexpr std::string_view MeshShader = R"(
#type vertex
#version 450 core
layout(location = 0) in vec3 a_Position;
layout(location = 1) in vec3 a_Normal;
layout(location = 2) in vec2 a_TexCoord;
uniform mat4 u_ViewProjection;
uniform mat4 u_Model;
layout(location = 0) out vec3 v_Normal;
layout(location = 1) out vec2 v_TexCoord;

void main()
{
    v_Normal = mat3(u_Model) * a_Normal;
    v_TexCoord = a_TexCoord;
    gl_Position = u_ViewProjection * u_Model * vec4(a_Position, 1.0);
}

#type fragment
#version 450 core
layout(location = 0) in vec3 v_Normal;
layout(location = 1) in vec2 v_TexCoord;
uniform bool u_WriteTexCoord;
layout(location = 0) out int o_Value;

void main()
{
    if (u_WriteTexCoord)
    {
        o_Value = int(v_TexCoord.x * 4.0) + 10 * int(v_TexCoord.y * 4.0);
    }
    else
    {
        o_Value = int(round(dot(normalize(v_Normal), vec3(1.0, 10.0, 100.0))));
    }
}
)";

    constexpr std::string_view PositionShader = R"(
#type vertex
#version 450 core
layout(location = 0) in vec2 a_Position;

void main()
{
    gl_Position = vec4(a_Position, 0.0, 1.0);
}

#type fragment
#version 450 core
layout(location = 0) out int o_Value;

void main()
{
    o_Value = 7;
}
)";

    class Scene
    {
    public:
        Scene() : m_Window(TestHelpers::CreateWindowWithContext())
        {
            REQUIRE(m_Window != nullptr);
            m_Shader = CreateShader(MeshShader);
            m_Framebuffer = GetContext().CreateFramebuffer(
                {.Width = 4, .Height = 4, .Attachments = {Quasar::FramebufferFormat::RedInteger, Quasar::FramebufferFormat::Depth24Stencil8}});
            REQUIRE(m_Framebuffer != nullptr);
            m_Camera.SetPosition({0.0f, 0.0f, 3.0f});
        }

        Quasar::GraphicsContext& GetContext() const { return *m_Window->GetGraphicsContext(); }

        Quasar::Scope<Quasar::Shader> CreateShader(std::string_view text) const
        {
            const std::optional<Quasar::ShaderSpecification> specification = Quasar::ShaderSpecification::Parse("Mesh", text);
            REQUIRE(specification.has_value());
            Quasar::Scope<Quasar::Shader> shader = GetContext().CreateShader(*specification);
            REQUIRE(shader != nullptr);
            return shader;
        }

        Quasar::Scope<Quasar::Mesh> CreateMesh(const Quasar::MeshData& data) const
        {
            Quasar::Scope<Quasar::Mesh> mesh = Quasar::Mesh::Create(GetContext(), data);
            REQUIRE(mesh != nullptr);
            return mesh;
        }

        std::vector<int> Draw(const Quasar::Mesh& mesh, CullMode culling, const glm::mat4& viewProjection, const glm::mat4& model = glm::mat4(1.0f),
                              bool writeTexCoord = false, Quasar::Shader* shader = nullptr) const
        {
            Quasar::Shader& target = shader != nullptr ? *shader : *m_Shader;
            const Quasar::Scope<Quasar::Pipeline> pipeline = GetContext().CreatePipeline({.Shader = &target, .Culling = culling});
            REQUIRE(pipeline != nullptr);
            if (shader == nullptr)
            {
                m_Shader->SetMat4("u_ViewProjection", viewProjection);
                m_Shader->SetMat4("u_Model", model);
                m_Shader->SetInt("u_WriteTexCoord", writeTexCoord ? 1 : 0);
            }

            m_Framebuffer->ClearInteger(0, -1);
            m_Framebuffer->ClearDepthStencil();
            m_Framebuffer->Bind();
            pipeline->Bind();
            GetContext().DrawIndexed(mesh.GetVertexArray());
            m_Framebuffer->Unbind();

            std::vector<int> values;
            for (std::uint32_t y = 0; y < 4; ++y)
            {
                for (std::uint32_t x = 0; x < 4; ++x)
                {
                    values.push_back(m_Framebuffer->ReadInteger(0, x, y).value_or(-2));
                }
            }
            return values;
        }

        int DrawCenter(const Quasar::Mesh& mesh, CullMode culling, const glm::mat4& model = glm::mat4(1.0f)) const
        {
            return Draw(mesh, culling, m_Camera.GetViewProjection(), model).at(5);
        }

    private:
        Quasar::Scope<Quasar::Window> m_Window;
        Quasar::Scope<Quasar::Shader> m_Shader;
        Quasar::Scope<Quasar::Framebuffer> m_Framebuffer;
        Quasar::Camera m_Camera;
    };
}

TEST_CASE("A quad that covers the image shows its texture coordinates upright and not mirrored")
{
    const TestHelpers::ScopedLog log;
    const Scene scene;
    const Quasar::Scope<Quasar::Mesh> quad = scene.CreateMesh(Quasar::Primitives::CreateQuad(2.0f));

    const std::vector<int> values = scene.Draw(*quad, CullMode::Back, glm::mat4(1.0f), glm::mat4(1.0f), true);

    CHECK(values == std::vector<int>{0, 1, 2, 3, 10, 11, 12, 13, 20, 21, 22, 23, 30, 31, 32, 33});
    CHECK(glGetError() == GL_NO_ERROR);
    CHECK_FALSE(TestHelpers::HasLogAtLeast(Quasar::LogLevel::Warning));
}

TEST_CASE("A cube and a sphere seen through a camera show the faces turned toward it")
{
    const TestHelpers::ScopedLog log;
    const Scene scene;
    const Quasar::Scope<Quasar::Mesh> cube = scene.CreateMesh(Quasar::Primitives::CreateCube());
    const Quasar::Scope<Quasar::Mesh> sphere = scene.CreateMesh(Quasar::Primitives::CreateSphere());
    const glm::mat4 turned = glm::mat4_cast(glm::angleAxis(glm::radians(90.0f), glm::vec3(1.0f, 0.0f, 0.0f)));

    CHECK(scene.DrawCenter(*cube, CullMode::Back) == 100);
    CHECK(scene.DrawCenter(*cube, CullMode::Front) == -100);
    CHECK(scene.DrawCenter(*cube, CullMode::Back, turned) == 100);
    CHECK(scene.DrawCenter(*sphere, CullMode::Back) > 0);
    CHECK(scene.DrawCenter(*sphere, CullMode::Front) < 0);
    CHECK(glGetError() == GL_NO_ERROR);
    CHECK_FALSE(TestHelpers::HasLogAtLeast(Quasar::LogLevel::Warning));
}

TEST_CASE("A mesh can have vertices of another layout than the standard vertex")
{
    const TestHelpers::ScopedLog log;
    const Scene scene;
    const Quasar::Scope<Quasar::Shader> shader = scene.CreateShader(PositionShader);
    const std::array<glm::vec2, 4> positions{glm::vec2(-1.0f, -1.0f), glm::vec2(1.0f, -1.0f), glm::vec2(1.0f, 1.0f), glm::vec2(-1.0f, 1.0f)};
    constexpr std::array<std::uint32_t, 6> indices{0, 1, 2, 0, 2, 3};

    const Quasar::Scope<Quasar::Mesh> mesh =
        Quasar::Mesh::Create(scene.GetContext(), std::as_bytes(std::span(positions)), {{0, Quasar::VertexAttributeType::Vec2}}, indices);

    REQUIRE(mesh != nullptr);
    CHECK(scene.Draw(*mesh, CullMode::Back, glm::mat4(1.0f), glm::mat4(1.0f), false, shader.get()) == std::vector<int>(16, 7));
    CHECK(glGetError() == GL_NO_ERROR);
}

TEST_CASE("A mesh without triangles, with an incomplete triangle or with an index that is not a vertex is refused")
{
    const Scene scene;
    const std::array<glm::vec2, 4> positions{};
    const std::span<const std::byte> vertices = std::as_bytes(std::span(positions));
    const Quasar::VertexLayout layout{{0, Quasar::VertexAttributeType::Vec2}};
    constexpr std::array<std::uint32_t, 4> incomplete{0, 1, 2, 3};
    constexpr std::array<std::uint32_t, 3> outside{0, 1, 4};
    constexpr std::array<std::uint32_t, 3> inside{0, 1, 3};

    {
        const TestHelpers::ScopedLog log;
        CHECK(Quasar::Mesh::Create(scene.GetContext(), vertices, layout, inside) != nullptr);
        CHECK_FALSE(TestHelpers::HasLogAtLeast(Quasar::LogLevel::Error));
    }
    for (const std::span<const std::uint32_t> indices :
         {std::span<const std::uint32_t>(), std::span<const std::uint32_t>(incomplete), std::span<const std::uint32_t>(outside)})
    {
        const TestHelpers::ScopedLog log;
        CAPTURE(indices.size());
        CHECK(Quasar::Mesh::Create(scene.GetContext(), vertices, layout, indices) == nullptr);
        CHECK(TestHelpers::HasLogAtLeast(Quasar::LogLevel::Error));
    }
    {
        const TestHelpers::ScopedLog log;
        CHECK(Quasar::Mesh::Create(scene.GetContext(), Quasar::Primitives::CreateCube(0.0f)) == nullptr);
        CHECK(Quasar::Mesh::Create(scene.GetContext(), {}, layout, inside) == nullptr);
        CHECK(TestHelpers::HasLogAtLeast(Quasar::LogLevel::Error));
    }
    CHECK(glGetError() == GL_NO_ERROR);
}
