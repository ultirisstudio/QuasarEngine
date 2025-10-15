#pragma once

#include "TextureTypes.h"

namespace QuasarEngine::TextureUtils
{
    inline const char* TextureWrapToChar(TextureWrap wrap) { return ToString(wrap).data(); }
    inline const char* TextureFilterToChar(TextureFilter f) { return ToString(f).data(); }
    inline const char* TextureFormatToChar(TextureFormat f) { return ToString(f).data(); }

    inline TextureWrap CharToTextureWrap(const char* s) {
        auto v = WrapFromString(s ? std::string_view{ s } : std::string_view{});
        return v.value_or(TextureWrap::REPEAT);
    }
    inline TextureFilter CharToTextureFilter(const char* s) {
        auto v = FilterFromString(s ? std::string_view{ s } : std::string_view{});
        return v.value_or(TextureFilter::NEAREST);
    }
    inline TextureFormat CharToTextureFormat(const char* s) {
        auto v = FormatFromString(s ? std::string_view{ s } : std::string_view{});
        return v.value_or(TextureFormat::RGBA);
    }
}