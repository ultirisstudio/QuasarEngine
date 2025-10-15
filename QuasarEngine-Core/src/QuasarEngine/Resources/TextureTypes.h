#pragma once

#include <cstdint>
#include <string_view>
#include <optional>

namespace QuasarEngine
{
    enum class TextureFormat : uint8_t {
        RED = 0,
        RED8,
        RGB,
        RGB8,
        RGBA,
        RGBA8,
        SRGB,
        SRGB8,
        SRGBA,
        SRGB8A8,
    };

    enum class TextureWrap : uint8_t {
        REPEAT = 0,
        MIRRORED_REPEAT,
        CLAMP_TO_EDGE,
        CLAMP_TO_BORDER
    };

    enum class TextureFilter : uint8_t {
        NEAREST = 0,
        LINEAR,
        NEAREST_MIPMAP_NEAREST,
        LINEAR_MIPMAP_NEAREST,
        NEAREST_MIPMAP_LINEAR,
        LINEAR_MIPMAP_LINEAR
    };

    struct TextureSpecification {
        TextureFormat format{ TextureFormat::RGBA };
        TextureFormat internal_format{ TextureFormat::RGBA };

        TextureWrap wrap_r{ TextureWrap::REPEAT };
        TextureWrap wrap_s{ TextureWrap::REPEAT };
        TextureWrap wrap_t{ TextureWrap::REPEAT };

        TextureFilter min_filter_param{ TextureFilter::LINEAR };
        TextureFilter mag_filter_param{ TextureFilter::LINEAR };

        uint32_t width{ 0 };
        uint32_t height{ 0 };

        bool alpha{ true };
        bool gamma{ false };
        bool flip{ true };
        bool mipmap{ true };
        bool compressed{ true };

        uint32_t Samples{ 1 };
        uint32_t channels{ 4 };

        constexpr bool is_srgb() const noexcept {
            return internal_format == TextureFormat::SRGB ||
                internal_format == TextureFormat::SRGB8 ||
                internal_format == TextureFormat::SRGBA ||
                internal_format == TextureFormat::SRGB8A8;
        }
    };

    constexpr std::string_view ToString(TextureWrap wrap) {
        switch (wrap) {
        case TextureWrap::REPEAT: return "Repeat";
        case TextureWrap::MIRRORED_REPEAT: return "Mirrored Repeat";
        case TextureWrap::CLAMP_TO_EDGE: return "Clamp to Edge";
        case TextureWrap::CLAMP_TO_BORDER: return "Clamp to Border";
        }
        return "Unknown";
    }

    constexpr std::string_view ToString(TextureFilter filter) {
        switch (filter) {
        case TextureFilter::NEAREST: return "Nearest";
        case TextureFilter::LINEAR: return "Linear";
        case TextureFilter::NEAREST_MIPMAP_NEAREST: return "Nearest Mipmap Nearest";
        case TextureFilter::LINEAR_MIPMAP_NEAREST: return "Linear Mipmap Nearest";
        case TextureFilter::NEAREST_MIPMAP_LINEAR: return "Nearest Mipmap Linear";
        case TextureFilter::LINEAR_MIPMAP_LINEAR: return "Linear Mipmap Linear";
        }
        return "Unknown";
    }

    constexpr std::string_view ToString(TextureFormat fmt) {
        switch (fmt) {
        case TextureFormat::RED: return "RED";
        case TextureFormat::RED8: return "RED8";
        case TextureFormat::RGB: return "RGB";
        case TextureFormat::RGB8: return "RGB8";
        case TextureFormat::RGBA: return "RGBA";
        case TextureFormat::RGBA8: return "RGBA8";
        case TextureFormat::SRGB: return "SRGB";
        case TextureFormat::SRGB8: return "SRGB8";
        case TextureFormat::SRGBA: return "SRGBA";
        case TextureFormat::SRGB8A8: return "SRGB8A8";
        }
        return "Unknown";
    }

    inline std::optional<TextureWrap> WrapFromString(std::string_view s) {
        if (s == "Repeat") return TextureWrap::REPEAT;
        if (s == "Mirrored Repeat" || s == "MirroredRepeat") return TextureWrap::MIRRORED_REPEAT;
        if (s == "Clamp to Edge" || s == "ClampToEdge") return TextureWrap::CLAMP_TO_EDGE;
        if (s == "Clamp to Border" || s == "ClampToBorder") return TextureWrap::CLAMP_TO_BORDER;
        return std::nullopt;
    }

    inline std::optional<TextureFilter> FilterFromString(std::string_view s) {
        if (s == "Nearest") return TextureFilter::NEAREST;
        if (s == "Linear") return TextureFilter::LINEAR;
        if (s == "Nearest Mipmap Nearest" || s == "NearestMipmapNearest") return TextureFilter::NEAREST_MIPMAP_NEAREST;
        if (s == "Linear Mipmap Nearest" || s == "LinearMipmapNearest")  return TextureFilter::LINEAR_MIPMAP_NEAREST;
        if (s == "Nearest Mipmap Linear" || s == "NearestMipmapLinear")  return TextureFilter::NEAREST_MIPMAP_LINEAR;
        if (s == "Linear Mipmap Linear" || s == "LinearMipmapLinear")   return TextureFilter::LINEAR_MIPMAP_LINEAR;
        return std::nullopt;
    }

    inline std::optional<TextureFormat> FormatFromString(std::string_view s) {
        if (s == "RED") return TextureFormat::RED;
        if (s == "RED8") return TextureFormat::RED8;
        if (s == "RGB") return TextureFormat::RGB;
        if (s == "RGB8") return TextureFormat::RGB8;
        if (s == "RGBA") return TextureFormat::RGBA;
        if (s == "RGBA8") return TextureFormat::RGBA8;
        if (s == "SRGB") return TextureFormat::SRGB;
        if (s == "SRGB8") return TextureFormat::SRGB8;
        if (s == "SRGBA") return TextureFormat::SRGBA;
        if (s == "SRGB8A8") return TextureFormat::SRGB8A8;
        return std::nullopt;
    }
}