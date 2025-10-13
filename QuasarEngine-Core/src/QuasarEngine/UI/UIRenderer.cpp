#include "qepch.h"

#include "UIDebug.h"
#include "UIStyle.h"
#include "UIRenderer.h"
#include "UITransform.h"

#include <QuasarEngine/Renderer/Buffer.h>
#include <QuasarEngine/Renderer/RenderCommand.h>
#include <QuasarEngine/Renderer/Renderer.h>
#include <QuasarEngine/Asset/AssetManager.h>
#include <QuasarEngine/Core/Logger.h>

namespace QuasarEngine {
    static bool DecodeUTF8(const char*& p, const char* end, uint32_t& cp) {
        if (p >= end) return false;
        unsigned char c0 = (unsigned char)*p++;
        if (c0 < 0x80) { cp = c0; return true; }
        if ((c0 >> 5) == 0x6) {
            if (p >= end) return false;
            unsigned char c1 = (unsigned char)*p++;
            cp = ((c0 & 0x1F) << 6) | (c1 & 0x3F);
            return true;
        }
        if ((c0 >> 4) == 0xE) {
            if (p + 1 >= end) return false;
            unsigned char c1 = (unsigned char)*p++;
            unsigned char c2 = (unsigned char)*p++;
            cp = ((c0 & 0x0F) << 12) | ((c1 & 0x3F) << 6) | (c2 & 0x3F);
            return true;
        }
        if ((c0 >> 3) == 0x1E) {
            if (p + 2 >= end) return false;
            unsigned char c1 = (unsigned char)*p++;
            unsigned char c2 = (unsigned char)*p++;
            unsigned char c3 = (unsigned char)*p++;
            cp = ((c0 & 0x07) << 18) | ((c1 & 0x3F) << 12) | ((c2 & 0x3F) << 6) | (c3 & 0x3F);
            return true;
        }
        return false;
    }

    static void UnpackColor(uint32_t rgba, float& r, float& g, float& b, float& a) {
        r = ((rgba >> 0) & 0xFF) / 255.0f;
        g = ((rgba >> 8) & 0xFF) / 255.0f;
        b = ((rgba >> 16) & 0xFF) / 255.0f;
        a = ((rgba >> 24) & 0xFF) / 255.0f;
    }

    static UITexture MakeWhiteTexOnce() {
        static bool s_initialized = false;
        if (!s_initialized) {
            auto bytes = std::make_shared<std::vector<unsigned char>>(4, 255);

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
            asset.type = AssetType::TEXTURE;
            asset.spec = spec;
            asset.size = (uint32_t)bytes->size();
            asset.data = bytes->data();
            asset.hold = bytes;

            AssetManager::Instance().loadAsset(asset);
            s_initialized = true;
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

        Shader::ShaderStageFlags uiUniformFlags = Shader::StageToBit(Shader::ShaderStageType::Vertex);

        desc.globalUniforms = {
            {"projection", Shader::ShaderUniformType::Mat4, sizeof(glm::mat4), offsetof(UIUniforms, proj), 0, 0, uiUniformFlags}
        };

        desc.objectUniforms = {
            {"model", Shader::ShaderUniformType::Mat4, sizeof(glm::mat4), offsetof(UIUniforms, model), 1, 0, uiUniformFlags}
        };

        desc.samplers = {
            {"uTexture", 1, 1, Shader::StageToBit(Shader::ShaderStageType::Fragment)}
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

        m_DefaultFont = std::make_unique<UIFont>();
        if (!m_DefaultFont->LoadTTF("Assets/Fonts/Monocraft.ttf", 16.f))
            Q_ERROR("Failed to load default font: Monocraft.ttf");
        else
            Q_INFO("Default font loaded: Monocraft.ttf");

        m_Context.defaultFont = m_DefaultFont.get();

        if (!m_Context.defaultFont)
            UI_DIAG_ERROR("UIRenderer: defaultFont is null.");

        m_Context.whiteTex = MakeWhiteTexOnce();
    }

    UIRenderer::~UIRenderer()
    {
        m_VertexArray.reset();
        m_VertexBuffer.reset();
		m_Shader.reset();

        m_DefaultFont.reset();
        m_Context.defaultFont = nullptr;
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

    void UIRenderContext::DrawText(const char* s, float x, float y, const UIColor& color)
    {
        if (!defaultFont) return;

        uint32_t rgba = PackRGBA8(color);
        UITexture tex; tex.id = defaultFont->GetTextureId();

        float penX = x;
        float penY = y + defaultFont->Ascent();
        unsigned int prev_cp = 0;

        const char* p = s;
        const char* end = s + std::strlen(s);

        while (p < end) {
            uint32_t cp = 0;
            if (!DecodeUTF8(p, end, cp)) break;

            if (cp == '\n') {
                penY += defaultFont->LineHeight();
                penX = x;
                prev_cp = 0;
                continue;
            }

            if (prev_cp) penX += defaultFont->GetKerning(prev_cp, cp) * defaultFont->GetScale();

            const UIFontGlyph* g = defaultFont->GetGlyph(cp);
            if (!g) {
                static std::unordered_set<uint32_t> s_LoggedMissing;
                if (!s_LoggedMissing.count(cp)) {
                    s_LoggedMissing.insert(cp);
                    char buf[64]; snprintf(buf, sizeof(buf), "UIFont: missing glyph U+%04X", cp);
                    UI_DIAG_WARN(buf);
                }

                g = defaultFont->GetGlyph((uint32_t)'?');
                if (!g) { prev_cp = 0; continue; }
            }

            if (g->w > 0.0f && g->h > 0.0f) {
                const float gx = penX + g->offsetX;
                const float gy = penY - g->offsetY;
                batcher->PushQuadUV(gx, gy, g->w, g->h, g->u0, g->v0, g->u1, g->v1, tex, rgba, nullptr);
            }

            penX += g->advance;
            prev_cp = cp;
        }
    }

    void UIRenderer::Begin(int fbW, int fbH) {
        if (fbW <= 0 || fbH <= 0) {
            UI_DIAG_WARN("UIRenderer::Begin: invalid framebuffer size.");
        }

        if (m_Context.whiteTex.id.empty())
            UI_DIAG_WARN("UIRenderer::Begin: whiteTex not initialized yet.");

        m_FbW = fbW; m_FbH = fbH;
        m_Batcher.Clear();
        m_Context.batcher = &m_Batcher;
        if (m_Context.whiteTex.id.empty()) m_Context.whiteTex = MakeWhiteTexOnce();
    }

    void UIRenderer::End() {
        FlushToEngine();
    }

    void UIRenderer::FlushToEngine() {
        if (!GetShader()) {
            UI_DIAG_ERROR("UIRenderer: shader is null.");
            return;
        }

        if (!m_VertexArray || !m_VertexBuffer || !m_VertexArray->GetIndexBuffer()) {
            UI_DIAG_ERROR("UIRenderer: vertex array/buffer/index buffer is null.");
            return;
		}

        const auto& V = m_Batcher.Vertices();
        const auto& I = m_Batcher.Indices();
        const auto& C = m_Batcher.Commands();

        if (V.empty() || I.empty() || C.empty()) {
            UI_DIAG_INFO("UIRenderer: nothing to draw (V/I/C empty).");
            return;
        }

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
            const std::string& wantedId = (!cmd.tex.id.empty() && AssetManager::Instance().isAssetLoaded(cmd.tex.id))
                ? cmd.tex.id
                : m_Context.whiteTex.id;

            /*const std::string& wantedId = (!Renderer::Instance().m_SceneData.m_UI->Renderer().Ctx().defaultFont->GetTextureId().empty() && AssetManager::Instance().isAssetLoaded(Renderer::Instance().m_SceneData.m_UI->Renderer().Ctx().defaultFont->GetTextureId()))
                ? Renderer::Instance().m_SceneData.m_UI->Renderer().Ctx().defaultFont->GetTextureId()
                : m_Context.whiteTex.id;*/

            auto tex = AssetManager::Instance().getAsset<Texture2D>(wantedId);
            if (!tex) {
                UI_DIAG_WARN(std::string("UIRenderer: fallback to white, missing: ") + wantedId);
                tex = AssetManager::Instance().getAsset<Texture2D>(m_Context.whiteTex.id);
            }

            GetShader()->SetTexture("uTexture", tex.get(), Shader::SamplerType::Sampler2D);

            /*std::string textureName = Renderer::Instance().m_SceneData.m_UI->Renderer().Ctx().defaultFont->GetTextureId();
            if (AssetManager::Instance().isAssetLoaded(textureName))
            {
                auto tex = AssetManager::Instance().getAsset<Texture2D>(textureName);
                GetShader()->SetTexture(
                    "uTexture",
                    tex.get(),
                    Shader::SamplerType::Sampler2D
                );
            }
            else
            {
                UI_DIAG_WARN(std::string("UIRenderer: texture not loaded: ") + cmd.tex.id);

                GetShader()->SetTexture(
                    "uTexture",
                    AssetManager::Instance().getAsset<Texture>(m_Context.whiteTex.id).get(),
                    Shader::SamplerType::Sampler2D
                );
            }*/

            /*if (AssetManager::Instance().isAssetLoaded(cmd.tex.id)) {
                auto tex = AssetManager::Instance().getAsset<Texture2D>(cmd.tex.id);
                GetShader()->SetTexture(
                    "uTexture",
                    tex.get(),
                    Shader::SamplerType::Sampler2D
                );
            }
            else {
                UI_DIAG_WARN(std::string("UIRenderer: texture not loaded: ") + cmd.tex.id);

                auto tex = AssetManager::Instance().getAsset<Texture2D>(m_Context.whiteTex.id);
                GetShader()->SetTexture(
                    "uTexture",
                    tex.get(),
                    Shader::SamplerType::Sampler2D
                );
            }*/

            if (!GetShader()->UpdateObject(nullptr)) {
                UI_DIAG_ERROR("UIRenderer: UpdateObject() failed.");
                return;
            }

            if (cmd.scissor.w > 0 && cmd.scissor.h > 0) {
                int x = cmd.scissor.x;
                int y = m_FbH - (cmd.scissor.y + cmd.scissor.h);
                int w = cmd.scissor.w;
                int h = cmd.scissor.h;

                int x0 = std::max(0, x);
                int y0 = std::max(0, y);
                int x1 = std::min(m_FbW, x + w);
                int y1 = std::min(m_FbH, y + h);
                int cw = std::max(0, x1 - x0);
                int ch = std::max(0, y1 - y0);

                if (cw > 0 && ch > 0) {
                    RenderCommand::Instance().EnableScissor(true);
                    RenderCommand::Instance().SetScissor((uint32_t)x0, (uint32_t)y0, (uint32_t)cw, (uint32_t)ch);
                }
                else {
                    RenderCommand::Instance().EnableScissor(false);
                }
            }
            else {
                RenderCommand::Instance().EnableScissor(false);
            }

            RenderCommand::Instance().DrawElements(DrawMode::TRIANGLES, cmd.idxCount, cmd.idxOffset);
        }

        RenderCommand::Instance().EnableScissor(false);

        m_VertexArray->Unbind();
    }
}