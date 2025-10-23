#include "qepch.h"

#include "OpenGLTexture2D.h"
#include "OpenGLTextureUtils.h"

#include <stb_image.h>

#include <QuasarEngine/File/FileUtils.h>
#include <QuasarEngine/Core/Logger.h>

namespace QuasarEngine
{
    namespace Utils
    {
        static bool EndsWithInsensitive(const std::string& s, const char* suf)
        {
            const size_t n = s.size(), m = std::strlen(suf);
            if (m > n) return false;
            for (size_t i = 0; i < m; ++i) {
                char a = (char)std::tolower((unsigned char)s[n - m + i]);
                char b = (char)std::tolower((unsigned char)suf[i]);
                if (a != b) return false;
            }
            return true;
        }

        static bool IsFloatInternal(TextureFormat f)
        {
            switch (f) {
            case TextureFormat::R16F:
            case TextureFormat::RG16F:
            case TextureFormat::RGB16F:
            case TextureFormat::RGBA16F:
            case TextureFormat::R32F:
            case TextureFormat::RGB32F:
            case TextureFormat::RGBA32F:
            case TextureFormat::R11G11B10F:
                return true;
            default: return false;
            }
        }
    }

	OpenGLTexture2D::OpenGLTexture2D(const TextureSpecification& specification) : Texture2D(specification) {}

	OpenGLTexture2D::~OpenGLTexture2D()
	{
		if (m_ID) glDeleteTextures(1, &m_ID);
	}

    bool OpenGLTexture2D::LoadFromPath(const std::string& path)
    {
        m_LastPath = path;
        auto bytes = FileUtils::ReadFileBinary(path);
        if (bytes.empty()) {
            Q_ERROR("OpenGLTexture2D: failed to read file: " + path);
            return false;
        }
        return LoadFromMemory(ByteView{ bytes.data(), bytes.size() });
    }

    bool OpenGLTexture2D::LoadFromMemory(ByteView data)
    {
        if (data.empty()) {
            Q_ERROR("OpenGLTexture2D: empty memory buffer");
            return false;
        }

        stbi_set_flip_vertically_on_load(m_Specification.flip);

        if (!m_Specification.compressed) {
            if (m_Specification.width == 0 || m_Specification.height == 0) {
                Q_ERROR("OpenGLTexture2D: raw upload requires width/height in specification");
                return false;
            }
            const int expectedChannels = Utils::DesiredChannels(m_Specification.internal_format);
            if (expectedChannels <= 0) {
                Q_ERROR("OpenGLTexture2D: unsupported/unknown internal format for raw upload");
                return false;
            }
            const std::size_t expectedSize =
                (size_t)m_Specification.width * (size_t)m_Specification.height * (size_t)expectedChannels;
            if (data.size != expectedSize) {
                Q_WARNING("OpenGLTexture2D: raw size (" + std::to_string(data.size) +
                    ") != w*h*c (" + std::to_string(expectedSize) + ")");
            }
            m_Specification.channels = (uint32_t)expectedChannels;
            return UploadPixelsDSA(data, false);
        }

        const bool wantFloat = Utils::IsFloatInternal(m_Specification.internal_format)
            || stbi_is_hdr_from_memory((const stbi_uc*)data.data, (int)data.size) != 0
            || Utils::EndsWithInsensitive(m_LastPath, ".hdr");

        if (wantFloat) {
            int w = 0, h = 0, n = 0;
            const int desired = Utils::DesiredChannels(m_Specification.internal_format);
            float* decoded = stbi_loadf_from_memory(
                (const stbi_uc*)data.data, (int)data.size,
                &w, &h, &n, desired > 0 ? desired : 0
            );
            if (!decoded) {
                Q_ERROR(std::string("stb_image (float) failed: ") + stbi_failure_reason());
                return false;
            }

            m_Specification.width = (uint32_t)w;
            m_Specification.height = (uint32_t)h;
            m_Specification.channels = (uint32_t)(desired > 0 ? desired : n);

            const std::size_t bytes = (std::size_t)w * (std::size_t)h * (std::size_t)m_Specification.channels * sizeof(float);
            const bool ok = UploadPixelsDSA(ByteView{ (const uint8_t*)decoded, bytes }, true);
            stbi_image_free(decoded);
            return ok;
        }
        else {
            int w = 0, h = 0, n = 0;
            const int desired = Utils::DesiredChannels(m_Specification.internal_format);

            unsigned char* decoded = stbi_load_from_memory(
                (const stbi_uc*)data.data, (int)data.size,
                &w, &h, &n, desired > 0 ? desired : 0
            );
            if (!decoded) {
                Q_ERROR(std::string("stb_image failed: ") + stbi_failure_reason());
                return false;
            }

            m_Specification.width = (uint32_t)w;
            m_Specification.height = (uint32_t)h;
            m_Specification.channels = (uint32_t)(desired > 0 ? desired : n);

            const std::size_t bytes = (std::size_t)w * (std::size_t)h * (std::size_t)m_Specification.channels;
            const bool ok = UploadPixelsDSA(ByteView{ decoded, bytes }, false);
            stbi_image_free(decoded);
            return ok;
        }
    }

    bool OpenGLTexture2D::LoadFromData(ByteView pixels)
    {
        if (pixels.empty()) {
            Q_ERROR("OpenGLTexture2D: no pixel data");
            return false;
        }
        if (m_Specification.width == 0 || m_Specification.height == 0) {
            Q_ERROR("OpenGLTexture2D: width/height must be set before LoadFromData()");
            return false;
        }
        return UploadPixelsDSA(pixels, Utils::IsFloatInternal(m_Specification.internal_format));
    }

    bool OpenGLTexture2D::AllocateStorage()
    {
        if (m_ID) { glDeleteTextures(1, &m_ID); m_ID = 0; m_Loaded = false; }

        const auto glInt = Utils::ToGLFormat(m_Specification.internal_format);
        if (glInt.internal == 0) {
            Q_ERROR("OpenGLTexture2D::AllocateStorage: invalid internal format");
            return false;
        }

        glCreateTextures(GL_TEXTURE_2D, 1, &m_ID);

        glTextureParameteri(m_ID, GL_TEXTURE_WRAP_S, Utils::ToGLWrap(m_Specification.wrap_s));
        glTextureParameteri(m_ID, GL_TEXTURE_WRAP_T, Utils::ToGLWrap(m_Specification.wrap_t));
        glTextureParameteri(m_ID, GL_TEXTURE_MIN_FILTER, Utils::ToGLFilter(m_Specification.min_filter_param));
        glTextureParameteri(m_ID, GL_TEXTURE_MAG_FILTER, Utils::ToGLFilter(m_Specification.mag_filter_param));

        if (glInt.channels == 1) {
            const GLint swizzle[4] = { GL_RED, GL_RED, GL_RED, GL_ONE };
            glTextureParameteriv(m_ID, GL_TEXTURE_SWIZZLE_RGBA, swizzle);
        }

        const uint32_t levels = Utils::CalcMipLevelsFromSpec(m_Specification);
        glTextureStorage2D(m_ID, (GLint)levels, glInt.internal,
            (GLint)m_Specification.width, (GLint)m_Specification.height);

        m_Loaded = true;
        return true;
    }

    bool OpenGLTexture2D::UploadPixelsDSA(ByteView pixels, bool pixelsAreFloat)
    {
        const auto glInt = Utils::ToGLFormat(m_Specification.internal_format);
        const auto glExt = Utils::ToGLFormat(m_Specification.format);

        if (glInt.internal == 0 || glExt.external == 0) {
            Q_ERROR("OpenGLTexture2D: unsupported texture format mapping");
            return false;
        }

        if (m_ID) { glDeleteTextures(1, &m_ID); m_ID = 0; m_Loaded = false; }

        glCreateTextures(GL_TEXTURE_2D, 1, &m_ID);

        glTextureParameteri(m_ID, GL_TEXTURE_WRAP_S, Utils::ToGLWrap(m_Specification.wrap_s));
        glTextureParameteri(m_ID, GL_TEXTURE_WRAP_T, Utils::ToGLWrap(m_Specification.wrap_t));
        glTextureParameteri(m_ID, GL_TEXTURE_MIN_FILTER, Utils::ToGLFilter(m_Specification.min_filter_param));
        glTextureParameteri(m_ID, GL_TEXTURE_MAG_FILTER, Utils::ToGLFilter(m_Specification.mag_filter_param));

        if (glInt.channels == 1) {
            const GLint swizzle[4] = { GL_RED, GL_RED, GL_RED, GL_ONE };
            glTextureParameteriv(m_ID, GL_TEXTURE_SWIZZLE_RGBA, swizzle);
        }

        const uint32_t levels = Utils::CalcMipLevelsFromSpec(m_Specification);
        glTextureStorage2D(m_ID, (GLint)levels, glInt.internal, (GLint)m_Specification.width, (GLint)m_Specification.height);

        glTextureParameteri(m_ID, GL_TEXTURE_BASE_LEVEL, 0);
        glTextureParameteri(m_ID, GL_TEXTURE_MAX_LEVEL, levels - 1);

        GLint oldUnpack = 4;
        glGetIntegerv(GL_UNPACK_ALIGNMENT, &oldUnpack);
        glPixelStorei(GL_UNPACK_ALIGNMENT, 1);

        const GLenum uploadType = pixelsAreFloat ? GL_FLOAT : glExt.type;
        glTextureSubImage2D(m_ID, 0, 0, 0, (GLint)m_Specification.width, (GLint)m_Specification.height, glExt.external, uploadType, pixels.data);

        if (m_Specification.mipmap && levels > 1)
            glGenerateTextureMipmap(m_ID);

        glTextureParameterf(m_ID, GL_TEXTURE_MAX_LOD, float(levels - 1));

        glPixelStorei(GL_UNPACK_ALIGNMENT, oldUnpack);

        if (m_Specification.auto_generate_mips && levels > 1) {
            glGenerateTextureMipmap(m_ID);
        }

        m_Loaded = true;
        return true;
    }

    void OpenGLTexture2D::GenerateMips()
    {
        if (!m_ID) return;
        glGenerateTextureMipmap(m_ID);
    }

    void OpenGLTexture2D::Bind(int index) const
    {
        if (!m_ID) return;
        glBindTextureUnit(index, m_ID);
    }

    void OpenGLTexture2D::Unbind() const
    {
        
    }
}
