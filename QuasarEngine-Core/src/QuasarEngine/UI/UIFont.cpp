#include "qepch.h"

#include "UIFont.h"

#include <fstream>
#include <algorithm>

#define STB_TRUETYPE_IMPLEMENTATION
#include <stb_truetype.h>

#include <QuasarEngine/Resources/Texture2D.h>
#include <QuasarEngine/Renderer/Renderer.h>
#include <QuasarEngine/Asset/AssetManager.h>

#include <QuasarEngine/Core/Logger.h>

namespace QuasarEngine
{
    const UIFontGlyph* UIFont::GetGlyph(unsigned int cp) const {
        auto it = m_Glyphs.find(cp);
        return (it == m_Glyphs.end()) ? nullptr : &it->second;
    }

	UIFont::UIFont() : m_Id("wtf"), m_AtlasW(0), m_AtlasH(0), m_Ascent(0), m_Descent(0), m_LineGap(0), m_Scale(1.f)
    {
    }

    bool UIFont::LoadTTF(const std::string& path, float pixelHeight, int atlasW, int atlasH) {
        std::vector<unsigned char> ttf;
        {
            std::ifstream ifs(path, std::ios::binary);
            if (!ifs)
            {
				Q_ERROR("Failed to open font file: " + path);
                return false;
            }
            ttf.assign(std::istreambuf_iterator<char>(ifs), {});
        }

        stbtt_fontinfo info{};
        if (!stbtt_InitFont(&info, ttf.data(), stbtt_GetFontOffsetForIndex(ttf.data(), 0)))
        {
			Q_ERROR("Failed to initialize font: " + path);
            return false;
        }

        m_Scale = stbtt_ScaleForPixelHeight(&info, pixelHeight);
        int a, d, lg; stbtt_GetFontVMetrics(&info, &a, &d, &lg);
        m_Ascent = a * m_Scale;
        m_Descent = d * m_Scale;
        m_LineGap = lg * m_Scale;

        m_AtlasW = atlasW; m_AtlasH = atlasH;
        std::vector<unsigned char> atlas(m_AtlasW * m_AtlasH, 0);

        int penX = 0, penY = 0, rowH = 0;
        auto placeGlyph = [&](unsigned int cp) {
            int w, h, xoff, yoff;
            unsigned char* bmp = stbtt_GetCodepointBitmap(&info, m_Scale, m_Scale, cp, &w, &h, &xoff, &yoff);
            if (!bmp) return;

            if (penX + w >= m_AtlasW) { penX = 0; penY += rowH; rowH = 0; }
            if (penY + h >= m_AtlasH) { stbtt_FreeBitmap(bmp, nullptr); return; }

            for (int j = 0; j < h; ++j)
                std::memcpy(&atlas[(penY + j) * m_AtlasW + penX], &bmp[j * w], w);

            int adv, lsb;
            stbtt_GetCodepointHMetrics(&info, cp, &adv, &lsb);

            UIFontGlyph g{};
            g.advance = adv * m_Scale;
            g.offsetX = (float)xoff;
            g.offsetY = (float)yoff;
            g.w = (float)w; g.h = (float)h;
            g.u0 = (float)penX / m_AtlasW; g.v0 = (float)penY / m_AtlasH;
            g.u1 = (float)(penX + w) / m_AtlasW; g.v1 = (float)(penY + h) / m_AtlasH;
            m_Glyphs[cp] = g;

            penX += w + 1; rowH = std::max(rowH, h);
            stbtt_FreeBitmap(bmp, nullptr);
            };

        for (unsigned int c = 32; c < 127; ++c) placeGlyph(c);

		TextureSpecification spec{};
        spec.width = m_AtlasW;
        spec.height = m_AtlasH;
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
        asset.size = (uint32_t)bytes->size();
        asset.hold = bytes;

        Renderer::m_SceneData.m_AssetManager->loadAsset(asset);

		m_Id = asset.id;
        
        return true;
    }
}