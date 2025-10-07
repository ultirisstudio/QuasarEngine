#include "qepch.h"

#include "UIFont.h"

#include <fstream>
#include <algorithm>
#include <cstring>

#define STB_TRUETYPE_IMPLEMENTATION
#include <stb_truetype.h>

#include <QuasarEngine/Resources/Texture2D.h>
#include <QuasarEngine/Renderer/Renderer.h>
#include <QuasarEngine/Asset/AssetManager.h>
#include <QuasarEngine/Core/Logger.h>

namespace QuasarEngine
{
    struct UIFont::Impl {
        std::string                 id;
        std::vector<unsigned char>  ttf;
        stbtt_fontinfo              info{};
        std::unordered_map<uint32_t, UIFontGlyph> glyphs;

        int   atlasW = 0, atlasH = 0;
        float ascent = 0.f, descent = 0.f, lineGap = 0.f, scale = 1.f;

        static bool ReadFile(const std::string& path, std::vector<unsigned char>& out) {
            std::ifstream f(path, std::ios::binary);
            if (!f) return false;
            out.assign(std::istreambuf_iterator<char>(f), {});
            return true;
        }

        void PackGlyph(uint32_t cp, std::vector<unsigned char>& atlas,
            int& penX, int& penY, int& rowH)
        {
            int w, h, xoff, yoff;
            unsigned char* bmp = stbtt_GetCodepointBitmap(&info, scale, scale, (int)cp, &w, &h, &xoff, &yoff);
            if (!bmp) return;

            if (penX + w >= atlasW) { penX = 0; penY += rowH; rowH = 0; }
            if (penY + h >= atlasH) { stbtt_FreeBitmap(bmp, nullptr); return; }

            for (int j = 0; j < h; ++j) {
                const int dstRow = (atlasH - 1) - (penY + j);
                std::memcpy(&atlas[dstRow * atlasW + penX], &bmp[j * w], w);
            }

            int adv, lsb;
            stbtt_GetCodepointHMetrics(&info, (int)cp, &adv, &lsb);

            UIFontGlyph g{};
            g.advance = adv * scale;
            g.offsetX = static_cast<float>(xoff);
            g.offsetY = -static_cast<float>(yoff);
            g.w = static_cast<float>(w);
            g.h = static_cast<float>(h);
            g.u0 = float(penX) / float(atlasW);
            g.v0 = float(penY) / float(atlasH);
            g.u1 = float(penX + w) / float(atlasW);
            g.v1 = float(penY + h) / float(atlasH);

            glyphs[cp] = g;

            penX += w + 1;
            rowH = std::max(rowH, h);

            stbtt_FreeBitmap(bmp, nullptr);
        }
    };

    UIFont::UIFont() : m_Impl(std::make_unique<Impl>()) {}
    UIFont::~UIFont() = default;

    bool UIFont::LoadTTF(const std::string& path, float pixelHeight, int atlasW, int atlasH)
    {
        if (!Impl::ReadFile(path, m_Impl->ttf)) {
            Q_ERROR("Failed to open font file: " + path);
            return false;
        }

        if (!stbtt_InitFont(&m_Impl->info, m_Impl->ttf.data(), stbtt_GetFontOffsetForIndex(m_Impl->ttf.data(), 0))) {
            Q_ERROR("Failed to initialize font: " + path);
            return false;
        }

        m_Impl->scale = stbtt_ScaleForPixelHeight(&m_Impl->info, pixelHeight);
        int a, d, lg;
        stbtt_GetFontVMetrics(&m_Impl->info, &a, &d, &lg);
        m_Impl->ascent = a * m_Impl->scale;
        m_Impl->descent = d * m_Impl->scale;
        m_Impl->lineGap = lg * m_Impl->scale;

        m_Impl->atlasW = atlasW; m_Impl->atlasH = atlasH;
        std::vector<unsigned char> atlas(m_Impl->atlasW * m_Impl->atlasH, 0);

        int penX = 0, penY = 0, rowH = 0;

        for (uint32_t c = 32; c <= 126; ++c) m_Impl->PackGlyph(c, atlas, penX, penY, rowH);
        for (uint32_t c = 160; c <= 255; ++c) m_Impl->PackGlyph(c, atlas, penX, penY, rowH);

        TextureSpecification spec{};
        spec.width = m_Impl->atlasW;
        spec.height = m_Impl->atlasH;
        spec.format = TextureFormat::RED;
        spec.internal_format = TextureFormat::RED8;
        spec.wrap_s = TextureWrap::CLAMP_TO_EDGE;
        spec.wrap_t = TextureWrap::CLAMP_TO_EDGE;
        spec.min_filter_param = TextureFilter::LINEAR;
        spec.mag_filter_param = TextureFilter::LINEAR;
        spec.mipmap = false;
        spec.gamma = false;
        spec.flip = false;
        spec.channels = 1;
        spec.compressed = false;
        spec.Samples = 1;

        AssetToLoad asset{};
        asset.id = "font:" + path;
        asset.type = AssetType::TEXTURE;
        asset.spec = spec;

        auto bytes = std::make_shared<std::vector<unsigned char>>(std::move(atlas));
        asset.data = bytes->data();
        asset.size = static_cast<uint32_t>(bytes->size());
        asset.hold = bytes;

        AssetManager::Instance().loadAsset(asset);

        m_Impl->id = asset.id;
        return true;
    }

    std::string UIFont::GetTextureId() const { return m_Impl->id; }
    float UIFont::Ascent()  const { return m_Impl->ascent; }
    float UIFont::Descent() const { return m_Impl->descent; }
    float UIFont::LineGap() const { return m_Impl->lineGap; }
    float UIFont::GetScale() const { return m_Impl->scale; }

    const UIFontGlyph* UIFont::GetGlyph(uint32_t codepoint) const {
        auto it = m_Impl->glyphs.find(codepoint);
        return (it == m_Impl->glyphs.end()) ? nullptr : &it->second;
    }

    float UIFont::GetKerning(uint32_t prev, uint32_t curr) const {
        if (!prev || !curr) return 0.f;
        const int k = stbtt_GetCodepointKernAdvance(&m_Impl->info, (int)prev, (int)curr);
        return k * m_Impl->scale;
    }

    bool UIFont::HasGlyph(uint32_t codepoint) const {
        return m_Impl->glyphs.find(codepoint) != m_Impl->glyphs.end();
    }

}