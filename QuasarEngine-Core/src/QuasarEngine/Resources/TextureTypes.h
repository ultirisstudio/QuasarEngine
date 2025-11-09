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

        R16F,
        RG16F,
        RGB16F,
        RGBA16F,
        R32F,
        RGB32F,
        RGBA32F,
        R11G11B10F,

        R32I,

        DEPTH24,
        DEPTH32F,
        DEPTH24STENCIL8,
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

    enum class TextureUsage : uint8_t {
        None = 0,
        ShaderRead = 1 << 0,
        RenderTarget = 1 << 1,
        Storage = 1 << 2,
        CopySrc = 1 << 3,
        CopyDst = 1 << 4
    };

    inline constexpr TextureUsage operator|(TextureUsage a, TextureUsage b) {
        return static_cast<TextureUsage>(static_cast<uint8_t>(a) | static_cast<uint8_t>(b));
    }

    inline constexpr TextureUsage operator&(TextureUsage a, TextureUsage b) {
        return static_cast<TextureUsage>(static_cast<uint8_t>(a) & static_cast<uint8_t>(b));
    }

    inline constexpr bool Any(TextureUsage u) { return static_cast<uint8_t>(u) != 0; }

    struct TextureSpecification {
        TextureFormat format{ TextureFormat::RGBA };
        TextureFormat internal_format{ TextureFormat::RGBA };

        TextureWrap  wrap_r{ TextureWrap::REPEAT };
        TextureWrap  wrap_s{ TextureWrap::REPEAT };
        TextureWrap  wrap_t{ TextureWrap::REPEAT };
        TextureFilter min_filter_param{ TextureFilter::LINEAR };
        TextureFilter mag_filter_param{ TextureFilter::LINEAR };

        uint32_t width{ 0 };
        uint32_t height{ 0 };

        uint32_t mip_levels{ 1 };
        bool     auto_generate_mips{ false };

        bool is_cube{ false };

        bool alpha{ true };
        bool gamma{ false };
        bool flip{ true };
        bool mipmap{ true };
        bool compressed{ true };
        bool async_upload{ false };

        uint32_t Samples{ 1 };

        uint32_t channels{ 4 };

        float anisotropy = 1.0f;

        TextureUsage usage{ TextureUsage::ShaderRead };

        constexpr bool is_srgb() const noexcept {
            switch (internal_format) {
            case TextureFormat::SRGB:
            case TextureFormat::SRGB8:
            case TextureFormat::SRGBA:
            case TextureFormat::SRGB8A8: return true;
            default: return false;
            }
        }
        constexpr bool is_depth() const noexcept {
            switch (internal_format) {
            case TextureFormat::DEPTH24:
            case TextureFormat::DEPTH32F:
            case TextureFormat::DEPTH24STENCIL8: return true;
            default: return false;
            }
        }

        static constexpr std::uint32_t bit_width32(std::uint32_t x) {
            return x ? 1u + bit_width32(x >> 1) : 0u;
        }
        
        static constexpr std::uint32_t ComputeDefaultMipLevels(std::uint32_t w, std::uint32_t h) noexcept {
            const std::uint32_t m = (w > h ? w : h);
            return m ? bit_width32(m) : 1u;
        }
    };

    constexpr std::string_view ToString(TextureWrap wrap) {
        switch (wrap) {
        case TextureWrap::REPEAT:          return "Repeat";
        case TextureWrap::MIRRORED_REPEAT: return "Mirrored Repeat";
        case TextureWrap::CLAMP_TO_EDGE:   return "Clamp to Edge";
        case TextureWrap::CLAMP_TO_BORDER: return "Clamp to Border";
        }
        return "Unknown";
    }

    constexpr std::string_view ToString(TextureFilter filter) {
        switch (filter) {
        case TextureFilter::NEAREST:               return "Nearest";
        case TextureFilter::LINEAR:                return "Linear";
        case TextureFilter::NEAREST_MIPMAP_NEAREST:return "Nearest Mipmap Nearest";
        case TextureFilter::LINEAR_MIPMAP_NEAREST: return "Linear Mipmap Nearest";
        case TextureFilter::NEAREST_MIPMAP_LINEAR: return "Nearest Mipmap Linear";
        case TextureFilter::LINEAR_MIPMAP_LINEAR:  return "Linear Mipmap Linear";
        }
        return "Unknown";
    }

    constexpr std::string_view ToString(TextureFormat fmt) {
        switch (fmt) {
        case TextureFormat::RED:    return "RED";
        case TextureFormat::RED8:   return "RED8";
        case TextureFormat::RGB:    return "RGB";
        case TextureFormat::RGB8:   return "RGB8";
        case TextureFormat::RGBA:   return "RGBA";
        case TextureFormat::RGBA8:  return "RGBA8";

        case TextureFormat::SRGB:    return "SRGB";
        case TextureFormat::SRGB8:   return "SRGB8";
        case TextureFormat::SRGBA:   return "SRGBA";
        case TextureFormat::SRGB8A8: return "SRGB8A8";

        case TextureFormat::R16F:     return "R16F";
        case TextureFormat::RG16F:    return "RG16F";
        case TextureFormat::RGB16F:   return "RGB16F";
        case TextureFormat::RGBA16F:  return "RGBA16F";
        case TextureFormat::R32F:     return "R32F";
        case TextureFormat::RGB32F:   return "RGB32F";
        case TextureFormat::RGBA32F:  return "RGBA32F";
        case TextureFormat::R11G11B10F: return "R11G11B10F";

        case TextureFormat::R32I:     return "R32I";

        case TextureFormat::DEPTH24:         return "DEPTH24";
        case TextureFormat::DEPTH32F:        return "DEPTH32F";
        case TextureFormat::DEPTH24STENCIL8: return "DEPTH24STENCIL8";
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
        if (s == "RED")   return TextureFormat::RED;
        if (s == "RED8")  return TextureFormat::RED8;
        if (s == "RGB")   return TextureFormat::RGB;
        if (s == "RGB8")  return TextureFormat::RGB8;
        if (s == "RGBA")  return TextureFormat::RGBA;
        if (s == "RGBA8") return TextureFormat::RGBA8;

        if (s == "SRGB")    return TextureFormat::SRGB;
        if (s == "SRGB8")   return TextureFormat::SRGB8;
        if (s == "SRGBA")   return TextureFormat::SRGBA;
        if (s == "SRGB8A8") return TextureFormat::SRGB8A8;

        if (s == "R16F")       return TextureFormat::R16F;
        if (s == "RG16F")      return TextureFormat::RG16F;
        if (s == "RGB16F")     return TextureFormat::RGB16F;
        if (s == "RGBA16F")    return TextureFormat::RGBA16F;
        if (s == "R32F")       return TextureFormat::R32F;
        if (s == "RGB32F")     return TextureFormat::RGB32F;
        if (s == "RGBA32F")    return TextureFormat::RGBA32F;
        if (s == "R11G11B10F") return TextureFormat::R11G11B10F;

        if (s == "R32I")       return TextureFormat::R32I;

        if (s == "DEPTH24")         return TextureFormat::DEPTH24;
        if (s == "DEPTH32F")        return TextureFormat::DEPTH32F;
        if (s == "DEPTH24STENCIL8") return TextureFormat::DEPTH24STENCIL8;

        return std::nullopt;
    }
}