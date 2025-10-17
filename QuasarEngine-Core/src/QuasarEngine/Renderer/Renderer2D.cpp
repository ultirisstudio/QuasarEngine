#include "qepch.h"
#include "Renderer2D.h"

#include <algorithm>
#include <cstring>

#include <QuasarEngine/Renderer/RendererAPI.h>
#include <QuasarEngine/Renderer/RenderCommand.h>
#include <QuasarEngine/Scene/BaseCamera.h>
#include <QuasarEngine/Resources/Texture2D.h>

#include <QuasarEngine/Shader/Shader.h>

#include <QuasarEngine/Renderer/Buffer.h>
#include <QuasarEngine/Renderer/VertexArray.h>

namespace QuasarEngine
{
    void Renderer2D::Initialize()
    {
        Shader::ShaderDescription desc;

        const auto api = RendererAPI::GetAPI();
        auto extFor = [](RendererAPI::API api, Shader::ShaderStageType s) {
            if (api == RendererAPI::API::Vulkan) {
                switch (s) {
                case Shader::ShaderStageType::Vertex:   return ".vert.spv";
                case Shader::ShaderStageType::Fragment: return ".frag.spv";
                default: return "";
                }
            }
            else {
                switch (s) {
                case Shader::ShaderStageType::Vertex:   return ".vert.glsl";
                case Shader::ShaderStageType::Fragment: return ".frag.glsl";
                default: return "";
                }
            }
            };

        const std::string basePath = (api == RendererAPI::API::Vulkan)
            ? "Assets/Shaders/vk/spv/"
            : "Assets/Shaders/gl/";
        const std::string name = "sprite";

        desc.modules = {
            Shader::ShaderModuleInfo{
                Shader::ShaderStageType::Vertex,
                basePath + name + extFor(api, Shader::ShaderStageType::Vertex),
                {
                    {0, Shader::ShaderIOType::Vec3, "inPosition", true, ""},
                    {1, Shader::ShaderIOType::Vec4, "inColor",    true, ""},
                    {2, Shader::ShaderIOType::Vec2, "inTexCoord", true, ""},
                    {3, Shader::ShaderIOType::Vec2, "inTiling",   true, ""},
                    {4, Shader::ShaderIOType::Vec2, "inOffset",   true, ""},
                }
            },
            Shader::ShaderModuleInfo{
                Shader::ShaderStageType::Fragment,
                basePath + name + extFor(api, Shader::ShaderStageType::Fragment),
                {}
            }
        };

        struct Global {
            glm::mat4 view;
            glm::mat4 proj;
        };

        struct SpriteObject
        {
            int useTexture;
            int _pad0;
            int _pad1;
            int _pad2;
        };

        const auto Flags = Shader::StageToBit(Shader::ShaderStageType::Vertex)
            | Shader::StageToBit(Shader::ShaderStageType::Fragment);

        desc.globalUniforms = {
            {"view",       Shader::ShaderUniformType::Mat4,  sizeof(glm::mat4), offsetof(Global, view), 0, 0, Flags},
            {"projection", Shader::ShaderUniformType::Mat4,  sizeof(glm::mat4), offsetof(Global, proj), 0, 0, Flags},
        };

        desc.objectUniforms = {
            {"useTexture", Shader::ShaderUniformType::Int,   sizeof(int),       offsetof(SpriteObject, useTexture), 1, 0, Flags},
        };

        desc.samplers = {
            {"albedo_texture", 1, 1, Shader::StageToBit(Shader::ShaderStageType::Fragment)}
        };

        desc.blendMode = Shader::BlendMode::AlphaBlend;
        desc.cullMode = Shader::CullMode::None;
        desc.fillMode = Shader::FillMode::Solid;
        desc.depthFunc = Shader::DepthFunc::Always;
        desc.depthTestEnable = false;
        desc.depthWriteEnable = false;
        desc.topology = Shader::PrimitiveTopology::TriangleList;
        desc.enableDynamicViewport = true;
        desc.enableDynamicScissor = true;

        m_Shader = Shader::Create(desc);

        m_VAO = VertexArray::Create();

        m_VBO = VertexBuffer::Create(sizeof(QuadVertex) * kMaxVertices);
        
        BufferLayout layout = {
            { ShaderDataType::Vec3, "inPosition" },
            { ShaderDataType::Vec4, "inColor" },
            { ShaderDataType::Vec2, "inTexCoord" },
            { ShaderDataType::Vec2, "inTiling" },
            { ShaderDataType::Vec2, "inOffset" },
        };
        m_VBO->SetLayout(layout);
        m_VAO->AddVertexBuffer(m_VBO);

        std::vector<uint32_t> indices; indices.reserve(kMaxIndices);
        for (uint32_t i = 0, v = 0; i < kMaxQuads; ++i, v += 4) {
            indices.push_back(v + 0);
            indices.push_back(v + 1);
            indices.push_back(v + 2);
            indices.push_back(v + 2);
            indices.push_back(v + 3);
            indices.push_back(v + 0);
        }
        m_IBO = IndexBuffer::Create(indices.data(), (uint32_t)indices.size());
        m_VAO->SetIndexBuffer(m_IBO);

        m_CPUVertices.reserve(kMaxVertices);
        m_CPUIndices.reserve(kMaxIndices);
        m_Queue.reserve(4096);
    }

    void Renderer2D::Shutdown()
    {
        m_IBO.reset();
        m_VBO.reset();
        m_VAO.reset();
        m_Shader.reset();
        m_CPUVertices.clear();
        m_CPUIndices.clear();
        m_Queue.clear();
        m_Stats = {};
    }

    void Renderer2D::BeginScene(const BaseCamera& camera)
    {
        ResetStats();
        m_CPUVertices.clear();
        m_CPUIndices.clear();
        m_Queue.clear();

        m_View = camera.getViewMatrix();
        m_Proj = camera.getProjectionMatrix();
    }

    void Renderer2D::Submit(const Quad2D& q)
    {
        DrawCmd c;
        c.texture = q.texture;
        c.order = q.order;
        c.transform = q.transform;
        c.color = q.color;
        c.uv = q.uv;
        c.tiling = q.tiling;
        c.offset = q.offset;
        m_Queue.emplace_back(c);
    }

    void Renderer2D::EndScene()
    {
        if (m_Queue.empty()) return;

        std::stable_sort(m_Queue.begin(), m_Queue.end(),
            [](const DrawCmd& a, const DrawCmd& b) {
                if (a.order != b.order) return a.order < b.order;
                return a.texture < b.texture;
            });

        Flush();
    }

    void Renderer2D::Flush()
    {
        m_Shader->Use();

        m_Shader->SetUniform("view", &m_View, sizeof(glm::mat4));
        m_Shader->SetUniform("projection", &m_Proj, sizeof(glm::mat4));
        if (!m_Shader->UpdateGlobalState()) { m_Shader->Unuse(); return; }

        Texture* currentTex = nullptr;
        m_CPUVertices.clear();
        m_CPUIndices.clear();

        auto FlushBatch = [&](bool resetAccum)
            {
                if (m_CPUVertices.empty()) return;

                m_VBO->Upload(m_CPUVertices.data(), (uint32_t)(m_CPUVertices.size() * sizeof(QuadVertex)));

                const uint32_t quadCount = (uint32_t)(m_CPUVertices.size() / 4);
                const uint32_t indexCount = quadCount * 6;

                m_VAO->Bind();

                int useTex = (currentTex != nullptr) ? 1 : 0;
                m_Shader->SetUniform("useTexture", &useTex, sizeof(int));
                m_Shader->SetTexture("albedo_texture", currentTex);
                if (!m_Shader->UpdateObject(nullptr)) { m_VAO->Unbind(); return; }

				RenderCommand::Instance().DrawElements(DrawMode::TRIANGLES, indexCount, 0, 0);
                m_Stats.drawCalls++;
                m_Stats.quadCount += quadCount;

                m_VAO->Unbind();

                if (resetAccum) { m_CPUVertices.clear(); m_CPUIndices.clear(); }
            };

        for (size_t i = 0; i < m_Queue.size(); ++i)
        {
            const DrawCmd& c = m_Queue[i];

            if (currentTex != c.texture && !m_CPUVertices.empty())
            {
                FlushBatch(true);
            }

            currentTex = c.texture;

            if ((m_CPUVertices.size() + 4) > kMaxVertices)
            {
                FlushBatch(true);
            }

            PushQuadCPU(m_CPUVertices, m_CPUIndices, c);
        }

        FlushBatch(true);

        m_Shader->Unuse();
    }

    void Renderer2D::PushQuadCPU(std::vector<QuadVertex>& vv,
        std::vector<uint32_t>& ii,
        const DrawCmd& c)
    {
        static const glm::vec3 P[4] = {
            {-0.5f, -0.5f, 0.0f},
            { 0.5f, -0.5f, 0.0f},
            { 0.5f,  0.5f, 0.0f},
            {-0.5f,  0.5f, 0.0f}
        };
        const glm::vec2 T[4] = {
            { c.uv.x, c.uv.y },
            { c.uv.z, c.uv.y },
            { c.uv.z, c.uv.w },
            { c.uv.x, c.uv.w }
        };

        const uint32_t base = (uint32_t)vv.size();
        for (int i = 0; i < 4; ++i)
        {
            glm::vec4 wp = c.transform * glm::vec4(P[i], 1.0f);
            QuadVertex v;
            v.pos = glm::vec3(wp);
            v.color = c.color;
            v.uv = T[i];
            v.tiling = c.tiling;
            v.offset = c.offset;
            vv.emplace_back(v);
        }
        ii.push_back(base + 0);
        ii.push_back(base + 1);
        ii.push_back(base + 2);
        ii.push_back(base + 2);
        ii.push_back(base + 3);
        ii.push_back(base + 0);
    }
}