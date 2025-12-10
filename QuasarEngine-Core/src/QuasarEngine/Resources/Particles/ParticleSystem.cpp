#include "qepch.h"
#include "ParticleSystem.h"

#include <glm/gtc/matrix_transform.hpp>

#include <QuasarEngine/Renderer/RendererAPI.h>
#include <QuasarEngine/Asset/AssetManager.h>

namespace QuasarEngine
{
    namespace
    {
        std::string ExtFor(RendererAPI::API api, Shader::ShaderStageType s)
        {
            if (api == RendererAPI::API::Vulkan)
            {
                switch (s)
                {
                case Shader::ShaderStageType::Vertex:      return ".vert.spv";
                case Shader::ShaderStageType::Fragment:    return ".frag.spv";
                default:                                   return "";
                }
            }
            else
            {
                switch (s)
                {
                case Shader::ShaderStageType::Vertex:      return ".vert.glsl";
                case Shader::ShaderStageType::Fragment:    return ".frag.glsl";
                default:                                   return "";
                }
            }
        }
    }

    ParticleSystem::ParticleSystem(const std::string& texturePath, std::size_t maxParticles)
        : m_MaxParticles(maxParticles)
    {
        TextureSpecification tspec;
        tspec.alpha = true;
        tspec.channels = 4;

        m_Texture = Texture2D::Create(tspec);
        m_Texture->LoadFromPath(texturePath);

        m_Particles.reserve(m_MaxParticles);

        InitMesh();

        const auto api = RendererAPI::GetAPI();
        const std::string basePath = (api == RendererAPI::API::Vulkan)
            ? "Assets/Shaders/vk/spv/"
            : "Assets/Shaders/gl/";

        InitShader(basePath);
    }

    void ParticleSystem::InitMesh()
    {
        std::vector<float> vertices =
        {
            -0.5f, -0.5f, 0.0f,   0.0f, 0.0f,
             0.5f, -0.5f, 0.0f,   1.0f, 0.0f,
             0.5f,  0.5f, 0.0f,   1.0f, 1.0f,
            -0.5f,  0.5f, 0.0f,   0.0f, 1.0f
        };

        std::vector<unsigned int> indices =
        {
            0, 1, 2,
            2, 3, 0
        };

        BufferLayout layout =
        {
            { ShaderDataType::Vec3, "inPosition" },
            { ShaderDataType::Vec2, "inTexCoord" }
        };

        m_QuadMesh = std::make_shared<Mesh>(
            vertices,
            indices,
            layout,
            DrawMode::TRIANGLES,
            std::nullopt
        );
    }

    void ParticleSystem::InitShader(const std::string& basePath)
    {
        Shader::ShaderDescription desc;

        const auto api = RendererAPI::GetAPI();
        const std::string name = "particle";

        std::string vertPath = basePath + name + ExtFor(api, Shader::ShaderStageType::Vertex);
        std::string fragPath = basePath + name + ExtFor(api, Shader::ShaderStageType::Fragment);

        desc.modules = {
            Shader::ShaderModuleInfo{
                Shader::ShaderStageType::Vertex,
                vertPath,
                "",
                {
                    {0, Shader::ShaderIOType::Vec3, "inPosition", true, ""},
                    {1, Shader::ShaderIOType::Vec2, "inTexCoord", true, ""}
                }
            },
            Shader::ShaderModuleInfo{
                Shader::ShaderStageType::Fragment,
                fragPath,
                "",
                {}
            }
        };

        struct alignas(16) GlobalUniforms
        {
            glm::mat4 view;
            glm::mat4 projection;
            glm::vec3 camera_position;
        };

        constexpr Shader::ShaderStageFlags globalStages =
            Shader::StageToBit(Shader::ShaderStageType::Vertex) |
            Shader::StageToBit(Shader::ShaderStageType::Fragment);

        desc.globalUniforms = {
            { "view",            Shader::ShaderUniformType::Mat4,  sizeof(glm::mat4),  offsetof(GlobalUniforms, view),            0, 0, globalStages },
            { "projection",      Shader::ShaderUniformType::Mat4,  sizeof(glm::mat4),  offsetof(GlobalUniforms, projection),      0, 0, globalStages },
            { "camera_position", Shader::ShaderUniformType::Vec3,  sizeof(glm::vec3),  offsetof(GlobalUniforms, camera_position), 0, 0, globalStages }
        };

        struct alignas(16) ObjectUniforms
        {
            glm::mat4 model;
            glm::vec4 color;
            float life;
            float maxLife;
        };

        constexpr Shader::ShaderStageFlags objectStages =
            Shader::StageToBit(Shader::ShaderStageType::Vertex) |
            Shader::StageToBit(Shader::ShaderStageType::Fragment);

        desc.objectUniforms = {
            { "model",   Shader::ShaderUniformType::Mat4,  sizeof(glm::mat4),  offsetof(ObjectUniforms, model),   1, 0, objectStages },
            { "color",   Shader::ShaderUniformType::Vec4,  sizeof(glm::vec4),  offsetof(ObjectUniforms, color),   1, 0, objectStages },
            { "life",    Shader::ShaderUniformType::Float, sizeof(float),      offsetof(ObjectUniforms, life),    1, 0, objectStages },
            { "maxLife", Shader::ShaderUniformType::Float, sizeof(float),      offsetof(ObjectUniforms, maxLife), 1, 0, objectStages }
        };

        desc.samplers = {
            { "particle_texture", 1, 1, Shader::StageToBit(Shader::ShaderStageType::Fragment) }
        };

        desc.blendMode = Shader::BlendMode::AlphaBlend;
        desc.cullMode = Shader::CullMode::None;
        desc.fillMode = Shader::FillMode::Solid;
        desc.depthFunc = Shader::DepthFunc::Less;
        desc.depthTestEnable = true;
        desc.depthWriteEnable = false;

        desc.topology = Shader::PrimitiveTopology::TriangleList;
        desc.enableDynamicViewport = true;
        desc.enableDynamicScissor = true;
        desc.enableDynamicLineWidth = false;

        m_Shader = Shader::Create(desc);
    }

    void ParticleSystem::Emit(const glm::vec3& position,
        const glm::vec3& velocity,
        float life,
        float startSize,
        float endSize,
        const glm::vec4& colorStart,
        const glm::vec4& colorEnd,
        float rotation)
    {
        Particle* slot = nullptr;
        for (auto& p : m_Particles)
        {
            if (p.life <= 0.0f)
            {
                slot = &p;
                break;
            }
        }

        if (!slot)
        {
            if (m_Particles.size() >= m_MaxParticles)
                return;

            m_Particles.emplace_back();
            slot = &m_Particles.back();
        }

        slot->position = position;
        slot->velocity = velocity;
        slot->maxLife = life;
        slot->life = life;
        slot->startSize = startSize;
        slot->endSize = endSize;
        slot->size = startSize;
        slot->colorStart = colorStart;
        slot->colorEnd = colorEnd;
        slot->rotation = rotation;
    }

    void ParticleSystem::Update(float dt)
    {
        for (auto& p : m_Particles)
        {
            if (p.life <= 0.0f)
                continue;

            p.life -= dt;
            if (p.life <= 0.0f)
                continue;

            p.position += p.velocity * dt;

            float t = 1.0f - glm::clamp(p.life / p.maxLife, 0.0f, 1.0f);
            p.size = glm::mix(p.startSize, p.endSize, t);
        }
    }

    glm::mat4 ParticleSystem::BuildBillboard(const glm::mat4& view,
        const glm::vec3& position,
        float size,
        float rotation)
    {
        glm::vec3 right(
            view[0][0],
            view[1][0],
            view[2][0]
        );
        glm::vec3 up(
            view[0][1],
            view[1][1],
            view[2][1]
        );
        glm::vec3 forward = glm::normalize(glm::cross(right, up));

        float c = std::cos(rotation);
        float s = std::sin(rotation);

        glm::vec3 rRot = right * c + up * s;
        glm::vec3 uRot = -right * s + up * c;

        glm::mat4 model(1.0f);
        model[0] = glm::vec4(rRot * size, 0.0f);
        model[1] = glm::vec4(uRot * size, 0.0f);
        model[2] = glm::vec4(forward * size, 0.0f);
        model[3] = glm::vec4(position, 1.0f);

        return model;
    }

    void ParticleSystem::Render(RenderContext& ctx)
    {
        if (!m_Shader || !m_QuadMesh)
            return;

        m_Shader->Use();

        m_Shader->SetUniform("view", &ctx.view, sizeof(glm::mat4));
        m_Shader->SetUniform("projection", &ctx.projection, sizeof(glm::mat4));
        m_Shader->SetUniform("camera_position", &ctx.cameraPosition, sizeof(glm::vec3));

        if (m_Texture.get())
        {
            m_Shader->SetTexture("particle_texture", m_Texture.get());
        }

        m_Shader->UpdateGlobalState();

        for (auto& p : m_Particles)
        {
            if (p.life <= 0.0f)
                continue;

            glm::mat4 model = BuildBillboard(ctx.view, p.position, p.size, p.rotation);

            float t = 1.0f - glm::clamp(p.life / p.maxLife, 0.0f, 1.0f);
            glm::vec4 color = glm::mix(p.colorStart, p.colorEnd, t);

            m_Shader->SetUniform("model", &model, sizeof(glm::mat4));
            m_Shader->SetUniform("color", &color, sizeof(glm::vec4));
            m_Shader->SetUniform("life", &p.life, sizeof(float));
            m_Shader->SetUniform("maxLife", &p.maxLife, sizeof(float));

            m_Shader->UpdateObject(nullptr);

            m_QuadMesh->draw();
        }

        m_Shader->Unuse();
    }
}
