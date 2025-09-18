#include "qepch.h"

#include "UIRenderer.h"
#include "UITransform.h"
#include "UIStyle.h"

#include <QuasarEngine/Renderer/Buffer.h>
#include <QuasarEngine/Renderer/RenderCommand.h>

namespace QuasarEngine {
    static void UnpackColor(uint32_t rgba, float& r, float& g, float& b, float& a) {
        r = ((rgba >> 0) & 0xFF) / 255.0f;
        g = ((rgba >> 8) & 0xFF) / 255.0f;
        b = ((rgba >> 16) & 0xFF) / 255.0f;
        a = ((rgba >> 24) & 0xFF) / 255.0f;
    }

    UIRenderer::UIRenderer()
    {
        Shader::ShaderDescription desc;

        std::string vertPath = "Assets/Shaders/ui.vert." + std::string(RendererAPI::GetAPI() == RendererAPI::API::Vulkan ? "spv" : "gl.glsl");
        std::string fragPath = "Assets/Shaders/ui.frag." + std::string(RendererAPI::GetAPI() == RendererAPI::API::Vulkan ? "spv" : "gl.glsl");

        desc.modules = {
            Shader::ShaderModuleInfo{
                Shader::ShaderStageType::Vertex,
                vertPath,
                {
                    {0, Shader::ShaderIOType::Vec2, "inPosition", true, ""},
                    {1, Shader::ShaderIOType::Vec2, "inTexCoord", true, ""},
                    {2, Shader::ShaderIOType::Vec4, "inColor",    true, ""}
                }
            },
            Shader::ShaderModuleInfo{
                Shader::ShaderStageType::Fragment,
                fragPath,
                {}
            }
        };

        struct UIUniforms {
            glm::mat4 proj;
            glm::mat4 model;
        };

        Shader::ShaderStageFlags uiUniformFlags =
            Shader::StageToBit(Shader::ShaderStageType::Vertex);

        desc.globalUniforms = {
            {"proj", Shader::ShaderUniformType::Mat4, sizeof(glm::mat4), offsetof(UIUniforms, proj), 0, 0, uiUniformFlags}
        };

        desc.objectUniforms = {
            {"model", Shader::ShaderUniformType::Mat4, sizeof(glm::mat4), offsetof(UIUniforms, model), 0, 0, uiUniformFlags}
        };
        desc.samplers = {};

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

		m_Shader->AcquireResources(&m_Material);

        m_VertexArray = VertexArray::Create();

        m_VertexBuffer = VertexBuffer::Create(64 * 1024);
        m_VertexBuffer->SetLayout({
            { ShaderDataType::Vec2, "inPosition" },
            { ShaderDataType::Vec2, "inTexCoord" },
            { ShaderDataType::Vec4, "inColor" }
            });

        m_VertexArray->AddVertexBuffer(m_VertexBuffer);

        std::shared_ptr<IndexBuffer> indexBuffer = IndexBuffer::Create(32 * 1024);
        m_VertexArray->SetIndexBuffer(indexBuffer);
    }

    UIRenderer::~UIRenderer()
    {
		m_Shader->ReleaseResources(&m_Material);

        m_VertexArray.reset();
        m_VertexBuffer.reset();
		m_Shader.reset();
    }

    void UIBatcher::Clear() {
        m_Vertices.clear(); m_Indices.clear(); m_Cmds.clear();
    }

    void UIBatcher::PushRect(const Rect& r, uint32_t rgba, const UIScissor* sc) {
        int v0 = (int)m_Vertices.size();
        m_Vertices.push_back({ r.x,       r.y,       0.f,0.f, rgba });
        m_Vertices.push_back({ r.x + r.w,   r.y,       0.f,0.f, rgba });
        m_Vertices.push_back({ r.x + r.w,   r.y + r.h,   0.f,0.f, rgba });
        m_Vertices.push_back({ r.x,       r.y + r.h,   0.f,0.f, rgba });
        int i0 = (int)m_Indices.size();
        m_Indices.push_back(v0 + 0); m_Indices.push_back(v0 + 1); m_Indices.push_back(v0 + 2);
        m_Indices.push_back(v0 + 0); m_Indices.push_back(v0 + 2); m_Indices.push_back(v0 + 3);
        UIDrawCmd cmd; cmd.vtxOffset = 0; cmd.idxOffset = i0; cmd.idxCount = 6;
        if (sc) cmd.scissor = *sc;
        m_Cmds.push_back(cmd);
    }

    void UIRenderContext::DrawDebugText(const char* s, float x, float y, const UIColor& color) {
        Rect r{ x, y, 8.f, 18.f };
        uint32_t rgba = PackRGBA8(color);
        for (const char* p = s; *p; ++p) {
            Rect bar{ r.x, r.y + r.h - 3.f, r.w, 2.f };
            batcher->PushRect(bar, rgba, nullptr);
            r.x += 8.f;
        }
    }

    void UIRenderer::Begin(int fbW, int fbH) {
        fbW_ = fbW; fbH_ = fbH;
        m_Batcher.Clear();
        m_Context.batcher = &m_Batcher;
    }

    void UIRenderer::End() {
        FlushToEngine();
    }

    void UIRenderer::FlushToEngine() {
        const auto& V = m_Batcher.Vertices();
        const auto& I = m_Batcher.Indices();
        const auto& C = m_Batcher.Commands();
        if (V.empty() || I.empty() || C.empty()) return;

        std::vector<float> interleaved;
        interleaved.reserve(V.size() * 8);
        for (const auto& v : V) {
            interleaved.push_back(v.x);
            interleaved.push_back(v.y);
            interleaved.push_back(v.u);
            interleaved.push_back(v.v);

            float r, g, b, a;
            UnpackColor(v.rgba, r, g, b, a);
            interleaved.push_back(r);
            interleaved.push_back(g);
            interleaved.push_back(b);
            interleaved.push_back(a);
        }

        const uint32_t vbBytes = (uint32_t)(interleaved.size() * sizeof(float));
        const uint32_t ibBytes = (uint32_t)(I.size() * sizeof(uint32_t));

        m_VertexArray->Bind();

        m_VertexBuffer->Reserve(vbBytes);
        m_VertexArray->GetIndexBuffer()->Reserve(ibBytes);

        m_VertexBuffer->Upload(interleaved.data(), vbBytes);
        m_VertexArray->GetIndexBuffer()->Upload(I.data(), ibBytes);

        for (const auto& cmd : C) {
            RenderCommand::DrawElements(DrawMode::TRIANGLES, cmd.idxCount, cmd.idxOffset);
        }

		m_VertexArray->Unbind();
    }
}