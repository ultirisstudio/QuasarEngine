#pragma once

#include <string>
#include <unordered_map>
#include <memory>
#include <vector>
#include <cstdint>

namespace QuasarEngine {

    struct UIFontGlyph {
        float advance;
        float offsetX, offsetY;
        float w, h;
        float u0, v0, u1, v1;
    };

    class UIFont {
    public:
        UIFont();
        ~UIFont();

        bool LoadTTF(const std::string& ttfPath,
            float pixelHeight,
            int atlasW = 512,
            int atlasH = 512);

        std::string GetTextureId() const;

        float Ascent()  const;
        float Descent() const;
        float LineGap() const;
        float GetScale() const;

        const UIFontGlyph* GetGlyph(uint32_t codepoint) const;
        float GetKerning(uint32_t prev, uint32_t curr) const;

        bool HasGlyph(uint32_t codepoint) const;

    private:
        struct Impl;
        std::unique_ptr<Impl> m_Impl;
    };

}