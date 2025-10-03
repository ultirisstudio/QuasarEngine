#include "qepch.h"

#include "UIRenderer.h"
#include "UITransform.h"
#include "UIStyle.h"

#include <QuasarEngine/Renderer/Buffer.h>
#include <QuasarEngine/Renderer/RenderCommand.h>
#include <QuasarEngine/Renderer/Renderer.h>
#include <QuasarEngine/Asset/AssetManager.h>
#include <QuasarEngine/Core/Logger.h>

namespace QuasarEngine {
    static void UnpackColor(uint32_t rgba, float& r, float& g, float& b, float& a) {
        r = ((rgba >> 0) & 0xFF) / 255.0f;
        g = ((rgba >> 8) & 0xFF) / 255.0f;
        b = ((rgba >> 16) & 0xFF) / 255.0f;
        a = ((rgba >> 24) & 0xFF) / 255.0f;
    }

    static UITexture MakeWhiteTexOnce() {
        static std::string id = "";
        if (id.empty()) {
			std::vector<unsigned char> data = { 255, 255, 255, 255 };

			TextureSpecification spec{};
			spec.width = 1;
			spec.height = 1;
			spec.format = TextureFormat::RGBA;
			spec.internal_format = TextureFormat::RGBA;
			spec.wrap_s = TextureWrap::CLAMP_TO_EDGE;
			spec.wrap_t = TextureWrap::CLAMP_TO_EDGE;
			spec.min_filter_param = TextureFilter::NEAREST;
			spec.mag_filter_param = TextureFilter::NEAREST;
			spec.mipmap = false;
			spec.gamma = false;
			spec.flip = false;
			spec.channels = 4;
			spec.compressed = false;
			spec.Samples = 1;

			AssetToLoad asset{};
			asset.id = "ui:white";
			asset.size = 4;
			asset.type = AssetType::TEXTURE;
			asset.spec = spec;
            asset.data = data.data();

			Renderer::m_SceneData.m_AssetManager->loadAsset(asset);
        }

        UITexture t;
        t.id = "ui:white";
        
        return t;
    }

    UIRenderer::UIRenderer()
    {
        Shader::ShaderDescription desc;

        std::string basePath;
        std::string vertExt;
        std::string fragExt;

        if (RendererAPI::GetAPI() == RendererAPI::API::Vulkan)
        {
            basePath = "Assets/Shaders/vk/spv/";
            vertExt = ".vert.spv";
            fragExt = ".frag.spv";
        }
        else
        {
            basePath = "Assets/Shaders/gl/";
            vertExt = ".vert.glsl";
            fragExt = ".frag.glsl";
        }

        std::string vertPath = basePath + "ui" + vertExt;
        std::string fragPath = basePath + "ui" + fragExt;

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
        desc.samplers = {
            {"uTexture", 1, 0, Shader::StageToBit(Shader::ShaderStageType::Fragment)}
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

		m_Context.defaultFont = new UIFont();
        if (!m_Context.defaultFont->LoadTTF("Assets/Fonts/Monocraft.ttf", 16.f))
        {
            Q_ERROR("Failed to load default font: Monocraft.ttf");
        }
        else
        {
			Q_INFO("Default font loaded: Monocraft.ttf");
        }
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

    void UIBatcher::PushRect(const Rect& r, UITexture tex, uint32_t rgba, const UIScissor* sc) {
        int v0 = (int)m_Vertices.size();
        m_Vertices.push_back({ r.x,       r.y,       0.f,0.f, rgba });
        m_Vertices.push_back({ r.x + r.w,   r.y,       0.f,0.f, rgba });
        m_Vertices.push_back({ r.x + r.w,   r.y + r.h,   0.f,0.f, rgba });
        m_Vertices.push_back({ r.x,       r.y + r.h,   0.f,0.f, rgba });
        int i0 = (int)m_Indices.size();
        m_Indices.push_back(v0 + 0); m_Indices.push_back(v0 + 1); m_Indices.push_back(v0 + 2);
        m_Indices.push_back(v0 + 0); m_Indices.push_back(v0 + 2); m_Indices.push_back(v0 + 3);
        UIDrawCmd cmd; cmd.vtxOffset = 0; cmd.idxOffset = i0; cmd.idxCount = 6;
        cmd.tex = tex;
        if (sc) cmd.scissor = *sc;
        m_Cmds.push_back(cmd);
    }

    void UIBatcher::PushQuadUV(float x, float y, float w, float h, float u0, float v0, float u1, float v1, UITexture tex, uint32_t rgba, const UIScissor* sc)
    {
        int v0i = (int)m_Vertices.size();
        m_Vertices.push_back({ x,     y,     u0,v0, rgba });
        m_Vertices.push_back({ x + w,   y,     u1,v0, rgba });
        m_Vertices.push_back({ x + w,   y + h,   u1,v1, rgba });
        m_Vertices.push_back({ x,     y + h,   u0,v1, rgba });

        int i0 = (int)m_Indices.size();
        m_Indices.push_back(v0i + 0); m_Indices.push_back(v0i + 1); m_Indices.push_back(v0i + 2);
        m_Indices.push_back(v0i + 0); m_Indices.push_back(v0i + 2); m_Indices.push_back(v0i + 3);

        UIDrawCmd cmd; cmd.vtxOffset = 0; cmd.idxOffset = i0; cmd.idxCount = 6;
        cmd.tex = tex; if (sc) cmd.scissor = *sc;
        m_Cmds.push_back(cmd);
    }

    void UIRenderContext::DrawText(const char* s, float x, float y, const UIColor& color) {
        if (!defaultFont)
        {
            return;
        }

        uint32_t rgba = PackRGBA8(color);
        UITexture tex;
        tex.id = defaultFont->GetTextureId();

        float penX = x;
        float penY = y + defaultFont->Ascent();

        for (const char* p = s; *p; ++p) {
            unsigned int cp = (unsigned char)*p;
            if (cp == '\n') { penY += (defaultFont->Ascent() - defaultFont->Descent() + defaultFont->LineGap()); penX = x; continue; }
            auto g = defaultFont->GetGlyph(cp);
            if (!g) continue;

            float gx = penX + g->offsetX;
            float gy = penY - g->offsetY;
            batcher->PushQuadUV(gx, gy, g->w, g->h, g->u0, g->v0, g->u1, g->v1, tex, rgba, nullptr);

            penX += g->advance;
        }
    }

    void UIRenderer::Begin(int fbW, int fbH) {
        fbW_ = fbW; fbH_ = fbH;
        m_Batcher.Clear();
        m_Context.batcher = &m_Batcher;
        if (m_Context.whiteTex.id.empty()) m_Context.whiteTex = MakeWhiteTexOnce();
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
            if (Renderer::m_SceneData.m_AssetManager->isAssetLoaded(cmd.tex.id))
            {
                GetShader()->SetTexture("uTexture", Renderer::m_SceneData.m_AssetManager->getAsset<Texture>(cmd.tex.id).get(), Shader::SamplerType::Sampler2D);
            }

            if (!GetShader()->UpdateObject(&GetMaterial()))
            {
                return;
            }

            RenderCommand::DrawElements(DrawMode::TRIANGLES, cmd.idxCount, cmd.idxOffset);
        }

		m_VertexArray->Unbind();
    }
}