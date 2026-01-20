#include "qepch.h"
#include "ParticleSystem.h"

#include <algorithm>

#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/constants.hpp>
#include <glm/gtx/norm.hpp>

#include <QuasarEngine/Renderer/RendererAPI.h>
#include <QuasarEngine/Renderer/RenderCommand.h>

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
                case Shader::ShaderStageType::Vertex:   return ".vert.spv";
                case Shader::ShaderStageType::Fragment: return ".frag.spv";
                default:                                return "";
                }
            }
            else
            {
                switch (s)
                {
                case Shader::ShaderStageType::Vertex:   return ".vert.glsl";
                case Shader::ShaderStageType::Fragment: return ".frag.glsl";
                default:                                return "";
                }
            }
        }
    }

    ParticleSystem::ParticleSystem(const std::string& texturePath,
        std::size_t maxParticles)
        : m_MaxParticles(maxParticles)
    {
        m_Particles.resize(m_MaxParticles);
        m_AliveIndices.reserve(m_MaxParticles);
        m_AliveDistances.reserve(m_MaxParticles);
        m_GPUBuffer.reserve(m_MaxParticles);

        TextureSpecification tspec{};
        tspec.alpha = true;
        tspec.flip = true;
        m_Texture = Texture2D::Create(tspec);
        if (m_Texture && !texturePath.empty())
            m_Texture->LoadFromPath(texturePath);

        InitMesh();

        const auto api = RendererAPI::GetAPI();
        const std::string basePath = (api == RendererAPI::API::Vulkan)
            ? "Assets/Shaders/vk/spv/"
            : "Assets/Shaders/gl/";

        InitShader(basePath);
    }

    void ParticleSystem::SetTexture(const std::string& texturePath)
    {
        if (!m_Texture)
        {
            TextureSpecification tspec{};
            tspec.alpha = true;
            tspec.flip = true;
            m_Texture = Texture2D::Create(tspec);
        }

        if (m_Texture && !texturePath.empty())
            m_Texture->LoadFromPath(texturePath);
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

        m_VertexArray = VertexArray::Create();
        m_VertexBuffer = VertexBuffer::Create(
            vertices.data(),
            static_cast<uint32_t>(vertices.size() * sizeof(float))
        );
        m_VertexBuffer->SetLayout(layout);
        m_VertexArray->AddVertexBuffer(m_VertexBuffer);

        m_IndexBuffer = IndexBuffer::Create(
            indices.data(),
            static_cast<uint32_t>(indices.size() * sizeof(unsigned int))
        );
        m_VertexArray->SetIndexBuffer(m_IndexBuffer);

        m_DrawMode = DrawMode::TRIANGLES;
        m_VertexCount = vertices.size() / 5;
        m_IndexCount = indices.size();
    }

    void ParticleSystem::InitShader(const std::string& basePath)
    {
        Shader::ShaderDescription desc;

        const auto api = RendererAPI::GetAPI();
        const std::string name = "particle_advanced";

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
            float     time;
        };

        constexpr Shader::ShaderStageFlags globalStages =
            Shader::StageToBit(Shader::ShaderStageType::Vertex) |
            Shader::StageToBit(Shader::ShaderStageType::Fragment);

        desc.globalUniforms = {
            { "view",            Shader::ShaderUniformType::Mat4,  sizeof(glm::mat4),  offsetof(GlobalUniforms, view),            0, 0, globalStages },
            { "projection",      Shader::ShaderUniformType::Mat4,  sizeof(glm::mat4),  offsetof(GlobalUniforms, projection),      0, 0, globalStages },
            { "camera_position", Shader::ShaderUniformType::Vec3,  sizeof(glm::vec3),  offsetof(GlobalUniforms, camera_position), 0, 0, globalStages },
            { "time",            Shader::ShaderUniformType::Float, sizeof(float),      offsetof(GlobalUniforms, time),            0, 0, globalStages }
        };

        struct alignas(16) ObjectUniforms
        {
            float sizeOverLifeExponent;
            float alphaOverLifeExponent;
            float softFade;
            float padding;
        };

        constexpr Shader::ShaderStageFlags objectStages =
            Shader::StageToBit(Shader::ShaderStageType::Vertex) |
            Shader::StageToBit(Shader::ShaderStageType::Fragment);

        desc.objectUniforms = {
            { "sizeOverLifeExponent",  Shader::ShaderUniformType::Float, sizeof(float), offsetof(ObjectUniforms, sizeOverLifeExponent),  0, 0, objectStages },
            { "alphaOverLifeExponent", Shader::ShaderUniformType::Float, sizeof(float), offsetof(ObjectUniforms, alphaOverLifeExponent), 0, 0, objectStages },
            { "softFade",              Shader::ShaderUniformType::Float, sizeof(float), offsetof(ObjectUniforms, softFade),              0, 0, objectStages }
        };

        Shader::ShaderStorageBufferDesc sb{};
        sb.name = "ParticlesBuffer";
        sb.size = sizeof(GPUParticle) * m_MaxParticles;
        sb.binding = 2;
        sb.stages = Shader::StageToBit(Shader::ShaderStageType::Vertex);
        desc.storageBuffers.push_back(sb);

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

    void ParticleSystem::Emit(const Particle& spawnData)
    {
        Particle* target = nullptr;

        for (auto& p : m_Particles)
        {
            if (p.age >= p.lifetime)
            {
                target = &p;
                break;
            }
        }

        if (!target)
        {
            target = &m_Particles[0];
        }

        *target = spawnData;
    }

    void ParticleSystem::Update(float dt)
    {
        if (dt <= 0.0f)
            return;

        m_Time += dt;

        m_AliveIndices.clear();
        m_AliveDistances.clear();

        for (std::size_t i = 0; i < m_Particles.size(); ++i)
        {
            Particle& p = m_Particles[i];

            p.age += dt;
            if (p.age >= p.lifetime)
                continue;

            p.velocity += m_Settings.gravity * dt;
            p.velocity += m_Settings.wind * dt;

            if (m_Settings.linearDrag > 0.0f)
            {
                float damping = 1.0f / (1.0f + m_Settings.linearDrag * dt);
                p.velocity *= damping;
            }

            if (m_Settings.turbulenceStrength > 0.0f)
            {
                float t = m_Time * m_Settings.turbulenceFrequency + p.random * 37.21f;
                float sx = std::sin(t) * std::cos(t * 0.7f);
                float sy = std::sin(t * 1.3f);
                float sz = std::cos(t * 0.5f);

                glm::vec3 turb = glm::vec3(sx, sy, sz) * m_Settings.turbulenceStrength;
                p.velocity += turb * dt * m_Settings.turbulenceScale;
            }

            p.position += p.velocity * dt;
            p.rotation += p.angularVelocity * dt;

            m_AliveIndices.push_back(i);
        }
    }

    void ParticleSystem::Render(RenderContext& ctx)
    {
        if (!m_Shader || !m_VertexArray)
            return;

        if (m_AliveIndices.empty())
            return;

        m_Shader->Use();

        m_Shader->SetUniform("view", &ctx.view, sizeof(glm::mat4));
        m_Shader->SetUniform("projection", &ctx.projection, sizeof(glm::mat4));
        m_Shader->SetUniform("camera_position", &ctx.cameraPosition, sizeof(glm::vec3));
        m_Shader->SetUniform("time", &m_Time, sizeof(float));
        m_Shader->UpdateGlobalState();

        float sizeExp = m_Settings.sizeOverLifeExponent;
        float alphaExp = m_Settings.alphaOverLifeExponent;
        float softFade = 1.0f;

        m_Shader->SetUniform("sizeOverLifeExponent", &sizeExp, sizeof(float));
        m_Shader->SetUniform("alphaOverLifeExponent", &alphaExp, sizeof(float));
        m_Shader->SetUniform("softFade", &softFade, sizeof(float));

        if (m_Texture)
            m_Shader->SetTexture("particle_texture", m_Texture.get());

        m_AliveDistances.resize(m_AliveIndices.size());
        for (std::size_t idx = 0; idx < m_AliveIndices.size(); ++idx)
        {
            const Particle& p = m_Particles[m_AliveIndices[idx]];
            m_AliveDistances[idx] = glm::length2(p.position - ctx.cameraPosition);
        }

        std::vector<std::size_t> sorted = m_AliveIndices;
        std::sort(sorted.begin(), sorted.end(),
            [&](std::size_t a, std::size_t b)
            {
                return glm::length2(m_Particles[a].position - ctx.cameraPosition) > glm::length2(m_Particles[b].position - ctx.cameraPosition);
            });

        m_GPUBuffer.clear();
        m_GPUBuffer.reserve(sorted.size());

        for (std::size_t idx : sorted)
        {
            const Particle& p = m_Particles[idx];

            if (p.age >= p.lifetime)
                continue;

            GPUParticle gp{};
            gp.position = p.position;
            gp.size = p.size;

            gp.colorStart = p.colorStart;
            gp.colorEnd = p.colorEnd;

            gp.age = p.age;
            gp.lifetime = p.lifetime;
            gp.rotation = p.rotation;
            gp.random = p.random;

            m_GPUBuffer.push_back(gp);
        }

        if (m_GPUBuffer.empty())
        {
            m_Shader->Unuse();
            return;
        }

        m_Shader->SetStorageBuffer(
            "ParticlesBuffer",
            m_GPUBuffer.data(),
            m_GPUBuffer.size() * sizeof(GPUParticle)
        );

        m_Shader->UpdateObject(nullptr);

        m_VertexArray->Bind();
        RenderCommand::Instance().DrawElementsInstanced(
            m_DrawMode,
            static_cast<uint32_t>(m_IndexCount),
            static_cast<uint32_t>(m_GPUBuffer.size())
        );

        m_Shader->Unuse();
    }
}
