#include "qepch.h"

#include "OpenGLTexture2D.h"
#include "OpenGLTextureUtils.h"

#include <stb_image.h>

#include <QuasarEngine/File/FileUtils.h>
#include <QuasarEngine/Core/Logger.h>

#include <algorithm>
#include <cctype>
#include <cstring>

namespace QuasarEngine
{
    namespace Utils
    {
        static bool EndsWithInsensitive(const std::string& s, const char* suf)
        {
            const size_t n = s.size(), m = std::strlen(suf);
            if (m > n) return false;
            for (size_t i = 0; i < m; ++i)
            {
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

    OpenGLTexture2D::OpenGLTexture2D(const TextureSpecification& specification)
        : Texture2D(specification)
    {
    }

    OpenGLTexture2D::~OpenGLTexture2D()
    {
        if (m_ID) glDeleteTextures(1, &m_ID);
        if (m_SamplerID) { glDeleteSamplers(1, &m_SamplerID); m_SamplerID = 0; }
    }

    static void ConfigureTextureFixedState(GLuint tex, const TextureSpecification& spec,
        const Utils::GLFormat& glInt, uint32_t levels)
    {
        glTextureParameteri(tex, GL_TEXTURE_WRAP_S, Utils::ToGLWrap(spec.wrap_s));
        glTextureParameteri(tex, GL_TEXTURE_WRAP_T, Utils::ToGLWrap(spec.wrap_t));
        glTextureParameteri(tex, GL_TEXTURE_MIN_FILTER, Utils::ToGLFilter(spec.min_filter_param));
        glTextureParameteri(tex, GL_TEXTURE_MAG_FILTER, Utils::ToGLFilter(spec.mag_filter_param));

        if (glInt.channels == 1) {
            const GLint swizzle[4] = { GL_RED, GL_RED, GL_RED, GL_ONE };
            glTextureParameteriv(tex, GL_TEXTURE_SWIZZLE_RGBA, swizzle);
        }

        glTextureParameteri(tex, GL_TEXTURE_BASE_LEVEL, 0);
        glTextureParameteri(tex, GL_TEXTURE_MAX_LEVEL, (GLint)(levels > 0 ? levels - 1 : 0));
        glTextureParameterf(tex, GL_TEXTURE_MAX_LOD, (levels > 0) ? float(levels - 1) : 0.0f);
    }

    static void ConfigureOrCreateSampler(GLuint& sampler, const TextureSpecification& spec)
    {
        if (!sampler) glCreateSamplers(1, &sampler);

        glSamplerParameteri(sampler, GL_TEXTURE_WRAP_S, Utils::ToGLWrap(spec.wrap_s));
        glSamplerParameteri(sampler, GL_TEXTURE_WRAP_T, Utils::ToGLWrap(spec.wrap_t));
        glSamplerParameteri(sampler, GL_TEXTURE_MIN_FILTER, Utils::ToGLFilter(spec.min_filter_param));
        glSamplerParameteri(sampler, GL_TEXTURE_MAG_FILTER, Utils::ToGLFilter(spec.mag_filter_param));

#ifdef GL_TEXTURE_MAX_ANISOTROPY_EXT
        if (::GLAD_GL_EXT_texture_filter_anisotropic && spec.anisotropy > 1.0f) {
            float maxAniso = 1.0f;
            glGetFloatv(GL_MAX_TEXTURE_MAX_ANISOTROPY_EXT, &maxAniso);
            const float a = std::min(spec.anisotropy, maxAniso);
            glSamplerParameterf(sampler, GL_TEXTURE_MAX_ANISOTROPY_EXT, a);
        }
#endif
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

        if (!m_Specification.compressed)
        {
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

        const bool wantFloat =
            Utils::IsFloatInternal(m_Specification.internal_format) ||
            stbi_is_hdr_from_memory((const stbi_uc*)data.data, (int)data.size) != 0 ||
            Utils::EndsWithInsensitive(m_LastPath, ".hdr");

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

            const std::size_t bytes =
                (std::size_t)w * (std::size_t)h * (std::size_t)m_Specification.channels * sizeof(float);
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

            const std::size_t bytes =
                (std::size_t)w * (std::size_t)h * (std::size_t)m_Specification.channels;
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

        const uint32_t levels = Utils::CalcMipLevelsFromSpec(m_Specification);
        glTextureStorage2D(m_ID, (GLint)levels, glInt.internal,
            (GLint)m_Specification.width, (GLint)m_Specification.height);

        ConfigureTextureFixedState(m_ID, m_Specification, glInt, levels);

        ConfigureOrCreateSampler(m_SamplerID, m_Specification);

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

        const uint32_t levels = Utils::CalcMipLevelsFromSpec(m_Specification);
        glTextureStorage2D(m_ID, (GLint)levels, glInt.internal,
            (GLint)m_Specification.width, (GLint)m_Specification.height);

        ConfigureTextureFixedState(m_ID, m_Specification, glInt, levels);

        ConfigureOrCreateSampler(m_SamplerID, m_Specification);

        GLint oldUnpack = 4;
        glGetIntegerv(GL_UNPACK_ALIGNMENT, &oldUnpack);
        glPixelStorei(GL_UNPACK_ALIGNMENT, 1);

        const GLenum uploadType = pixelsAreFloat ? GL_FLOAT : glExt.type;

        if (m_Specification.async_upload)
        {
            GLuint pbo = 0;
            glCreateBuffers(1, &pbo);
            glNamedBufferData(pbo, (GLsizeiptr)pixels.size, nullptr, GL_STREAM_DRAW);

            void* dst = glMapNamedBufferRange(pbo, 0, (GLsizeiptr)pixels.size,
                GL_MAP_WRITE_BIT | GL_MAP_INVALIDATE_BUFFER_BIT);
            if (dst) {
                std::memcpy(dst, pixels.data, pixels.size);
                glUnmapNamedBuffer(pbo);
            }

            glBindBuffer(GL_PIXEL_UNPACK_BUFFER, pbo);
            glTextureSubImage2D(
                m_ID, 0, 0, 0,
                (GLint)m_Specification.width, (GLint)m_Specification.height,
                glExt.external, uploadType, (const void*)0 /* offset */
            );
            glBindBuffer(GL_PIXEL_UNPACK_BUFFER, 0);
            glDeleteBuffers(1, &pbo);
        }
        else
        {
            glTextureSubImage2D(
                m_ID, 0, 0, 0,
                (GLint)m_Specification.width, (GLint)m_Specification.height,
                glExt.external, uploadType, pixels.data
            );
        }

        glPixelStorei(GL_UNPACK_ALIGNMENT, oldUnpack);

        if (m_Specification.auto_generate_mips && levels > 1)
            glGenerateTextureMipmap(m_ID);

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
        if (m_SamplerID) glBindSampler(index, m_SamplerID);
    }

    void OpenGLTexture2D::Unbind() const
    {
        
    }
}