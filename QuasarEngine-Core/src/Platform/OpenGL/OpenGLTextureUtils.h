#pragma once

#include <QuasarEngine/Resources/TextureTypes.h>
#include <glad/glad.h>
#include <cmath>
#include <algorithm>

namespace QuasarEngine
{
    namespace Utils
    {
        struct GLFormat {
            GLenum internal = 0;
            GLenum external = 0;
            GLenum type = GL_UNSIGNED_BYTE;
            GLint  channels = 0;
        };

        static inline GLenum TargetFromSamples(uint32_t samples)
        {
            return samples > 1 ? GL_TEXTURE_2D_MULTISAMPLE : GL_TEXTURE_2D;
        }

        static inline GLenum TargetFromSpec(const TextureSpecification& spec)
        {
            if (spec.is_cube)              return GL_TEXTURE_CUBE_MAP;
            if (spec.Samples > 1)          return GL_TEXTURE_2D_MULTISAMPLE;
            return GL_TEXTURE_2D;
        }

        static inline GLenum CubeFaceFromLayer(uint32_t layer)
        {
            return GL_TEXTURE_CUBE_MAP_POSITIVE_X + layer;
        }

        static inline GLFormat ToGLFormat(TextureFormat fmt) {
            GLFormat out{};

            switch (fmt) {
            case TextureFormat::RED:
            case TextureFormat::RED8:
                out.internal = GL_R8;         out.external = GL_RED;              out.type = GL_UNSIGNED_BYTE; out.channels = 1; break;

            case TextureFormat::RGB:
            case TextureFormat::RGB8:
                out.internal = GL_RGB8;       out.external = GL_RGB;              out.type = GL_UNSIGNED_BYTE; out.channels = 3; break;

            case TextureFormat::RGBA:
            case TextureFormat::RGBA8:
                out.internal = GL_RGBA8;      out.external = GL_RGBA;             out.type = GL_UNSIGNED_BYTE; out.channels = 4; break;

            case TextureFormat::SRGB:
            case TextureFormat::SRGB8:
                out.internal = GL_SRGB8;      out.external = GL_RGB;              out.type = GL_UNSIGNED_BYTE; out.channels = 3; break;

            case TextureFormat::SRGBA:
            case TextureFormat::SRGB8A8:
                out.internal = GL_SRGB8_ALPHA8; out.external = GL_RGBA;           out.type = GL_UNSIGNED_BYTE; out.channels = 4; break;

            case TextureFormat::R16F:
                out.internal = GL_R16F;       out.external = GL_RED;              out.type = GL_HALF_FLOAT;    out.channels = 1; break;
            case TextureFormat::RG16F:
                out.internal = GL_RG16F;      out.external = GL_RG;               out.type = GL_HALF_FLOAT;    out.channels = 2; break;
            case TextureFormat::RGB16F:
                out.internal = GL_RGB16F;     out.external = GL_RGB;              out.type = GL_HALF_FLOAT;    out.channels = 3; break;
            case TextureFormat::RGBA16F:
                out.internal = GL_RGBA16F;    out.external = GL_RGBA;             out.type = GL_HALF_FLOAT;    out.channels = 4; break;

            case TextureFormat::R32F:
                out.internal = GL_R32F;       out.external = GL_RED;              out.type = GL_FLOAT;         out.channels = 1; break;
            case TextureFormat::RGB32F:
                out.internal = GL_RGB32F;     out.external = GL_RGB;              out.type = GL_FLOAT;         out.channels = 3; break;
            case TextureFormat::RGBA32F:
                out.internal = GL_RGBA32F;    out.external = GL_RGBA;             out.type = GL_FLOAT;         out.channels = 4; break;

            case TextureFormat::R11G11B10F:
                out.internal = GL_R11F_G11F_B10F; out.external = GL_RGB;
                out.type = GL_UNSIGNED_INT_10F_11F_11F_REV; out.channels = 3; break;

            case TextureFormat::R32I:
                out.internal = GL_R32I;       out.external = GL_RED_INTEGER;      out.type = GL_INT;           out.channels = 1; break;

            case TextureFormat::DEPTH24:
                out.internal = GL_DEPTH_COMPONENT24; out.external = GL_DEPTH_COMPONENT; out.type = GL_UNSIGNED_INT;          out.channels = 1; break;
            case TextureFormat::DEPTH32F:
                out.internal = GL_DEPTH_COMPONENT32F; out.external = GL_DEPTH_COMPONENT; out.type = GL_FLOAT;                 out.channels = 1; break;
            case TextureFormat::DEPTH24STENCIL8:
                out.internal = GL_DEPTH24_STENCIL8; out.external = GL_DEPTH_STENCIL;    out.type = GL_UNSIGNED_INT_24_8;     out.channels = 1; break;

            default:
                out.internal = GL_RGBA8; out.external = GL_RGBA; out.type = GL_UNSIGNED_BYTE; out.channels = 4;
                break;
            }
            return out;
        }

        static inline GLenum ToGLWrap(TextureWrap wrap) {
            switch (wrap) {
            case TextureWrap::REPEAT:          return GL_REPEAT;
            case TextureWrap::MIRRORED_REPEAT: return GL_MIRRORED_REPEAT;
            case TextureWrap::CLAMP_TO_EDGE:   return GL_CLAMP_TO_EDGE;
            case TextureWrap::CLAMP_TO_BORDER: return GL_CLAMP_TO_BORDER;
            }
            return GL_REPEAT;
        }

        static inline GLenum ToGLFilter(TextureFilter f) {
            switch (f) {
            case TextureFilter::NEAREST:               return GL_NEAREST;
            case TextureFilter::LINEAR:                return GL_LINEAR;
            case TextureFilter::NEAREST_MIPMAP_NEAREST:return GL_NEAREST_MIPMAP_NEAREST;
            case TextureFilter::LINEAR_MIPMAP_NEAREST: return GL_LINEAR_MIPMAP_NEAREST;
            case TextureFilter::NEAREST_MIPMAP_LINEAR: return GL_NEAREST_MIPMAP_LINEAR;
            case TextureFilter::LINEAR_MIPMAP_LINEAR:  return GL_LINEAR_MIPMAP_LINEAR;
            }
            return GL_LINEAR;
        }

        static inline GLint DesiredChannels(TextureFormat fmt) {
            switch (fmt) {
            case TextureFormat::RED:
            case TextureFormat::RED8:
            case TextureFormat::R16F:
            case TextureFormat::R32F:
            case TextureFormat::R32I:
                return 1;

            case TextureFormat::RG16F:
                return 2;

            case TextureFormat::RGB:
            case TextureFormat::RGB8:
            case TextureFormat::SRGB:
            case TextureFormat::SRGB8:
            case TextureFormat::RGB16F:
            case TextureFormat::RGB32F:
            case TextureFormat::R11G11B10F:
                return 3;

            case TextureFormat::RGBA:
            case TextureFormat::RGBA8:
            case TextureFormat::SRGBA:
            case TextureFormat::SRGB8A8:
            case TextureFormat::RGBA16F:
            case TextureFormat::RGBA32F:
                return 4;

            case TextureFormat::DEPTH24:
            case TextureFormat::DEPTH32F:
            case TextureFormat::DEPTH24STENCIL8:
                return 1;

            default:
                return 0;
            }
        }

        static inline GLint CalcMipmapLevels(GLint w, GLint h, bool mipmap) {
            if (!mipmap) return 1;
            GLint maxDim = (w > h) ? w : h;
            GLint levels = 1;
            while ((maxDim >>= 1) > 0) ++levels;
            return levels;
        }

        static inline uint32_t CalcMipLevelsFromSpec(const TextureSpecification& spec) {
            if (spec.mip_levels > 1) return spec.mip_levels;
            if (spec.mipmap || spec.auto_generate_mips) {
                const uint32_t m = std::max(spec.width, spec.height);
                if (m == 0) return 1;
                return 1u + (uint32_t)std::floor(std::log2((double)m));
            }
            return 1;
        }
    }
}