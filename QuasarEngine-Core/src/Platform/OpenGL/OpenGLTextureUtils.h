#pragma once

#include <QuasarEngine/Resources/TextureTypes.h>
#include <glad/glad.h>

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

		static GLenum  TargetFromSamples(uint32_t samples) { return samples > 1 ? GL_TEXTURE_2D_MULTISAMPLE : GL_TEXTURE_2D; }

		static GLFormat ToGLFormat(TextureFormat fmt) {
			GLFormat out{};
			switch (fmt) {
			case TextureFormat::RED:
			case TextureFormat::RED8:
				out.internal = GL_R8;      out.external = GL_RED;  out.channels = 1; break;
			case TextureFormat::RGB:
			case TextureFormat::RGB8:
			case TextureFormat::SRGB:
			case TextureFormat::SRGB8:
				out.internal = (fmt == TextureFormat::SRGB || fmt == TextureFormat::SRGB8) ? GL_SRGB8 : GL_RGB8;
				out.external = GL_RGB;     out.channels = 3; break;
			case TextureFormat::RGBA:
			case TextureFormat::RGBA8:
			case TextureFormat::SRGBA:
			case TextureFormat::SRGB8A8:
				out.internal = (fmt == TextureFormat::SRGBA || fmt == TextureFormat::SRGB8A8) ? GL_SRGB8_ALPHA8 : GL_RGBA8;
				out.external = GL_RGBA;    out.channels = 4; break;
			default: break;
			}
			return out;
		}

		static GLenum ToGLWrap(TextureWrap wrap) {
			switch (wrap) {
			case TextureWrap::REPEAT:          return GL_REPEAT;
			case TextureWrap::MIRRORED_REPEAT: return GL_MIRRORED_REPEAT;
			case TextureWrap::CLAMP_TO_EDGE:   return GL_CLAMP_TO_EDGE;
			case TextureWrap::CLAMP_TO_BORDER: return GL_CLAMP_TO_BORDER;
			}
			return GL_REPEAT;
		}

		static GLenum ToGLFilter(TextureFilter f) {
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

		static GLint DesiredChannels(TextureFormat fmt) {
			switch (fmt) {
			case TextureFormat::RED:
			case TextureFormat::RED8: return 1;
			case TextureFormat::RGB:
			case TextureFormat::RGB8:
			case TextureFormat::SRGB:
			case TextureFormat::SRGB8: return 3;
			case TextureFormat::RGBA:
			case TextureFormat::RGBA8:
			case TextureFormat::SRGBA:
			case TextureFormat::SRGB8A8: return 4;
			default: return 0;
			}
		}

		static GLint CalcMipmapLevels(GLint w, GLint h, bool mipmap) {
			if (!mipmap) return 1;
			GLint maxDim = (w > h) ? w : h;
			GLint levels = 1;
			while ((maxDim >>= 1) > 0) ++levels;
			return levels;
		}
	}
}