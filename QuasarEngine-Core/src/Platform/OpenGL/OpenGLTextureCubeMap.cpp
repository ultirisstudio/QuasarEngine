#include "qepch.h"

#include "OpenGLTextureCubeMap.h"
#include "OpenGLTextureUtils.h"

#include <stb_image.h>
#include <glad/glad.h>
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
            case TextureFormat::R16F: case TextureFormat::RG16F:
            case TextureFormat::RGB16F: case TextureFormat::RGBA16F:
            case TextureFormat::R32F: case TextureFormat::RGB32F:
            case TextureFormat::RGBA32F: case TextureFormat::R11G11B10F:
                return true;
            default: return false;
            }
        }

        static GLint FaceIndex(TextureCubeMap::Face f) { return static_cast<GLint>(f); }
    }

    OpenGLTextureCubeMap::OpenGLTextureCubeMap(const TextureSpecification& specification)
        : TextureCubeMap(specification)
    {
        m_Specification.is_cube = true;
    }

    OpenGLTextureCubeMap::~OpenGLTextureCubeMap()
    {
        if (m_ID) glDeleteTextures(1, &m_ID);
    }

    bool OpenGLTextureCubeMap::AllocateStorage(uint32_t w, uint32_t h)
    {
        if (m_StorageAllocated && m_Specification.width == w && m_Specification.height == h)
            return true;

        if (m_ID) { glDeleteTextures(1, &m_ID); m_ID = 0; m_Loaded = false; }
        glCreateTextures(GL_TEXTURE_CUBE_MAP, 1, &m_ID);

        glTextureParameteri(m_ID, GL_TEXTURE_WRAP_S, Utils::ToGLWrap(m_Specification.wrap_s));
        glTextureParameteri(m_ID, GL_TEXTURE_WRAP_T, Utils::ToGLWrap(m_Specification.wrap_t));
        glTextureParameteri(m_ID, GL_TEXTURE_WRAP_R, Utils::ToGLWrap(m_Specification.wrap_r));
        glTextureParameteri(m_ID, GL_TEXTURE_MIN_FILTER, Utils::ToGLFilter(m_Specification.min_filter_param));
        glTextureParameteri(m_ID, GL_TEXTURE_MAG_FILTER, Utils::ToGLFilter(m_Specification.mag_filter_param));

        const auto glInt = Utils::ToGLFormat(m_Specification.internal_format);
        if (glInt.internal == 0) {
            Q_ERROR("OpenGLTextureCubeMap: unsupported internal format");
            return false;
        }

        if (glInt.channels == 1) {
            const GLint swizzle[4] = { GL_RED, GL_RED, GL_RED, GL_ONE };
            glTextureParameteriv(m_ID, GL_TEXTURE_SWIZZLE_RGBA, swizzle);
        }

        m_Specification.width = w;
        m_Specification.height = h;

        const uint32_t levels = Utils::CalcMipLevelsFromSpec(m_Specification);
        glTextureStorage2D(m_ID, (GLint)levels, glInt.internal, (GLint)w, (GLint)h);

        glTextureParameteri(m_ID, GL_TEXTURE_BASE_LEVEL, 0);
        glTextureParameterf(m_ID, GL_TEXTURE_MAX_LOD, float(levels - 1));
        glTextureParameteri(m_ID, GL_TEXTURE_MAX_LEVEL, (GLint)levels - 1);

        m_StorageAllocated = true;
        m_FacesUploaded = 0;
        return true;
    }

    void OpenGLTextureCubeMap::GenerateMips()
    {
        if (!m_ID) return;
        glGenerateTextureMipmap(m_ID);
    }

    bool OpenGLTextureCubeMap::UploadAllFacesDSA(ByteView allFacesPixels, bool pixelsAreFloat)
    {
        const auto glExt = Utils::ToGLFormat(m_Specification.format);
        const auto glInt = Utils::ToGLFormat(m_Specification.internal_format);
        if (glExt.external == 0 || glInt.internal == 0) {
            Q_ERROR("OpenGLTextureCubeMap: unsupported format mapping");
            return false;
        }

        GLint oldUnpack = 4;
        glGetIntegerv(GL_UNPACK_ALIGNMENT, &oldUnpack);
        glPixelStorei(GL_UNPACK_ALIGNMENT, 1);

        const GLenum uploadType = pixelsAreFloat ? GL_FLOAT : glExt.type;

        glTextureSubImage3D(
            m_ID, 0,
            0, 0, 0,
            (GLint)m_Specification.width,
            (GLint)m_Specification.height,
            6,
            glExt.external, uploadType,
            allFacesPixels.data
        );

        glPixelStorei(GL_UNPACK_ALIGNMENT, oldUnpack);

        const uint32_t levels = Utils::CalcMipLevelsFromSpec(m_Specification);
        if (m_Specification.auto_generate_mips && levels > 1) {
            glGenerateTextureMipmap(m_ID);
        }

        m_FacesUploaded = 6;
        m_Loaded = true;
        return true;
    }

    bool OpenGLTextureCubeMap::UploadFaceDSA(Face face, ByteView facePixels, uint32_t w, uint32_t h, bool pixelsAreFloat)
    {
        const auto glExt = Utils::ToGLFormat(m_Specification.format);
        if (glExt.external == 0) {
            Q_ERROR("OpenGLTextureCubeMap: unsupported external format");
            return false;
        }

        GLint oldUnpack = 4;
        glGetIntegerv(GL_UNPACK_ALIGNMENT, &oldUnpack);
        glPixelStorei(GL_UNPACK_ALIGNMENT, 1);

        const GLenum uploadType = pixelsAreFloat ? GL_FLOAT : glExt.type;

        glTextureSubImage3D(
            m_ID, 0,
            0, 0, Utils::FaceIndex(face),
            (GLint)w,
            (GLint)h,
            1,
            glExt.external, uploadType,
            facePixels.data
        );

        glPixelStorei(GL_UNPACK_ALIGNMENT, oldUnpack);

        if (++m_FacesUploaded == 6) {
            const uint32_t levels = Utils::CalcMipLevelsFromSpec(m_Specification);
            if (m_Specification.auto_generate_mips && levels > 1) {
                glGenerateTextureMipmap(m_ID);
            }
            m_Loaded = true;
        }
        return true;
    }

    bool OpenGLTextureCubeMap::LoadFromPath(const std::string& path)
    {
        auto bytes = FileUtils::ReadFileBinary(path);
        if (bytes.empty()) {
            Q_ERROR("OpenGLTextureCubeMap: failed to read file: " + path);
            return false;
        }
        return LoadFromMemory(ByteView{ bytes.data(), bytes.size() });
    }

    bool OpenGLTextureCubeMap::LoadFromMemory(ByteView data)
    {
        if (data.empty()) {
            Q_ERROR("OpenGLTextureCubeMap: empty memory buffer");
            return false;
        }

        stbi_set_flip_vertically_on_load(m_Specification.flip);

        const bool wantFloat = Utils::IsFloatInternal(m_Specification.internal_format)
            || stbi_is_hdr_from_memory((const stbi_uc*)data.data, (int)data.size) != 0;

        if (m_Specification.format == TextureFormat::RGBA && m_Specification.internal_format == TextureFormat::RGBA) {
            if (m_Specification.alpha) {
                m_Specification.format = TextureFormat::RGBA;
                m_Specification.internal_format = m_Specification.gamma ? TextureFormat::SRGB8A8 : TextureFormat::RGBA8;
            }
            else {
                m_Specification.format = TextureFormat::RGB;
                m_Specification.internal_format = m_Specification.gamma ? TextureFormat::SRGB8 : TextureFormat::RGB8;
            }
        }

        int w = 0, h = 0, n = 0;
        const int desired = Utils::DesiredChannels(m_Specification.internal_format);

        if (wantFloat) {
            float* decoded = stbi_loadf_from_memory(
                (const stbi_uc*)data.data, (int)data.size,
                &w, &h, &n, desired > 0 ? desired : 0
            );
            if (!decoded) {
                Q_ERROR(std::string("OpenGLTextureCubeMap: stb_image (float) decode failed: ") + stbi_failure_reason());
                return false;
            }

            m_Specification.width = (uint32_t)w;
            m_Specification.height = (uint32_t)h;
            m_Specification.channels = (uint32_t)(desired > 0 ? desired : n);

            if (!AllocateStorage((uint32_t)w, (uint32_t)h)) { stbi_image_free(decoded); return false; }

            const std::size_t faceBytes = (size_t)w * (size_t)h * (size_t)m_Specification.channels * sizeof(float);
            bool ok = true;
            for (int fi = 0; fi < 6; ++fi) {
                ok &= UploadFaceDSA((Face)fi,
                    ByteView{ (const uint8_t*)decoded, faceBytes }, (uint32_t)w, (uint32_t)h, true);
            }
            stbi_image_free(decoded);
            return ok;
        }
        else {
            unsigned char* decoded = stbi_load_from_memory(
                (const stbi_uc*)data.data, (int)data.size,
                &w, &h, &n, desired > 0 ? desired : 0
            );
            if (!decoded) {
                Q_ERROR(std::string("OpenGLTextureCubeMap: stb_image decode failed: ") + stbi_failure_reason());
                return false;
            }

            m_Specification.width = (uint32_t)w;
            m_Specification.height = (uint32_t)h;
            m_Specification.channels = (uint32_t)(desired > 0 ? desired : n);

            if (!AllocateStorage((uint32_t)w, (uint32_t)h)) { stbi_image_free(decoded); return false; }

            const std::size_t faceBytes = (size_t)w * (size_t)h * (size_t)m_Specification.channels;
            bool ok = true;
            for (int fi = 0; fi < 6; ++fi) {
                ok &= UploadFaceDSA((Face)fi,
                    ByteView{ decoded, faceBytes }, (uint32_t)w, (uint32_t)h, false);
            }
            stbi_image_free(decoded);
            return ok;
        }
    }

    bool OpenGLTextureCubeMap::LoadFromData(ByteView data)
    {
        if (data.empty()) {
            Q_ERROR("OpenGLTextureCubeMap: no pixel data");
            return false;
        }
        if (m_Specification.width == 0 || m_Specification.height == 0) {
            Q_ERROR("OpenGLTextureCubeMap: width/height must be set before LoadFromData()");
            return false;
        }

        const GLint channels = Utils::DesiredChannels(m_Specification.internal_format);
        if (channels == 0) {
            Q_ERROR("OpenGLTextureCubeMap: invalid channels for internal format");
            return false;
        }

        if (!AllocateStorage(m_Specification.width, m_Specification.height))
            return false;

        const bool pixelsAreFloat = Utils::IsFloatInternal(m_Specification.internal_format);
        const std::size_t faceSize = (size_t)m_Specification.width * (size_t)m_Specification.height *
            (size_t)channels * (pixelsAreFloat ? sizeof(float) : sizeof(uint8_t));

        if (data.size == faceSize * 6) {
            return UploadAllFacesDSA(data, pixelsAreFloat);
        }
        else if (data.size == faceSize) {
            bool ok = true;
            for (int fi = 0; fi < 6; ++fi) {
                ok &= UploadFaceDSA((Face)fi, data,
                    m_Specification.width, m_Specification.height, pixelsAreFloat);
            }
            return ok;
        }
        else {
            Q_ERROR("OpenGLTextureCubeMap: data size must be faceSize or 6*faceSize");
            return false;
        }
    }

    bool OpenGLTextureCubeMap::LoadFaceFromPath(Face face, const std::string& path)
    {
        auto bytes = FileUtils::ReadFileBinary(path);
        if (bytes.empty()) {
            Q_ERROR("OpenGLTextureCubeMap: failed to read face file: " + path);
            return false;
        }
        return LoadFaceFromMemory(face, ByteView{ bytes.data(), bytes.size() });
    }

    bool OpenGLTextureCubeMap::LoadFaceFromMemory(Face face, ByteView data)
    {
        if (data.empty()) {
            Q_ERROR("OpenGLTextureCubeMap: empty memory buffer (face)");
            return false;
        }

        stbi_set_flip_vertically_on_load(m_Specification.flip);

        const bool wantFloat = Utils::IsFloatInternal(m_Specification.internal_format)
            || stbi_is_hdr_from_memory((const stbi_uc*)data.data, (int)data.size) != 0;

        int w = 0, h = 0, n = 0;
        const int desired = Utils::DesiredChannels(m_Specification.internal_format);

        if (wantFloat) {
            float* decoded = stbi_loadf_from_memory(
                (const stbi_uc*)data.data, (int)data.size, &w, &h, &n, desired > 0 ? desired : 0);
            if (!decoded) {
                Q_ERROR(std::string("OpenGLTextureCubeMap: stb_image face(float) failed: ") + stbi_failure_reason());
                return false;
            }

            if (!m_StorageAllocated) {
                m_Specification.width = (uint32_t)w;
                m_Specification.height = (uint32_t)h;
                m_Specification.channels = (uint32_t)(desired > 0 ? desired : n);
                if (!AllocateStorage(m_Specification.width, m_Specification.height)) {
                    stbi_image_free(decoded); return false;
                }
            }
            else {
                if (m_Specification.width != (uint32_t)w || m_Specification.height != (uint32_t)h) {
                    Q_ERROR("OpenGLTextureCubeMap: face dimension mismatch");
                    stbi_image_free(decoded); return false;
                }
            }

            const std::size_t bytes = (size_t)w * (size_t)h * (size_t)m_Specification.channels * sizeof(float);
            const bool ok = UploadFaceDSA(face, ByteView{ (const uint8_t*)decoded, bytes },
                (uint32_t)w, (uint32_t)h, true);
            stbi_image_free(decoded);
            return ok;
        }
        else {
            unsigned char* decoded = stbi_load_from_memory(
                (const stbi_uc*)data.data, (int)data.size, &w, &h, &n, desired > 0 ? desired : 0);
            if (!decoded) {
                Q_ERROR(std::string("OpenGLTextureCubeMap: stb_image face decode failed: ") + stbi_failure_reason());
                return false;
            }

            if (!m_StorageAllocated) {
                m_Specification.width = (uint32_t)w;
                m_Specification.height = (uint32_t)h;
                m_Specification.channels = (uint32_t)(desired > 0 ? desired : n);
                if (!AllocateStorage(m_Specification.width, m_Specification.height)) {
                    stbi_image_free(decoded); return false;
                }
            }
            else {
                if (m_Specification.width != (uint32_t)w || m_Specification.height != (uint32_t)h) {
                    Q_ERROR("OpenGLTextureCubeMap: face dimension mismatch");
                    stbi_image_free(decoded); return false;
                }
            }

            const std::size_t bytes = (size_t)w * (size_t)h * (size_t)m_Specification.channels;
            const bool ok = UploadFaceDSA(face, ByteView{ decoded, bytes },
                (uint32_t)w, (uint32_t)h, false);
            stbi_image_free(decoded);
            return ok;
        }
    }

    bool OpenGLTextureCubeMap::LoadFaceFromData(Face face, ByteView data, uint32_t w, uint32_t h, uint32_t channels)
    {
        if (data.empty() || w == 0 || h == 0 || channels == 0) {
            Q_ERROR("OpenGLTextureCubeMap: invalid face data");
            return false;
        }

        const GLint expected = Utils::DesiredChannels(m_Specification.internal_format);
        if (expected != 0 && (int)channels != expected) {
            Q_ERROR("OpenGLTextureCubeMap: channels mismatch with internal format");
            return false;
        }

        if (!m_StorageAllocated) {
            m_Specification.width = w;
            m_Specification.height = h;
            m_Specification.channels = channels;
            if (!AllocateStorage(w, h)) return false;
        }
        else {
            if (m_Specification.width != w || m_Specification.height != h) {
                Q_ERROR("OpenGLTextureCubeMap: face dimensions mismatch with allocated storage");
                return false;
            }
        }

        const bool pixelsAreFloat = Utils::IsFloatInternal(m_Specification.internal_format);
        return UploadFaceDSA(face, data, w, h, pixelsAreFloat);
    }

    void OpenGLTextureCubeMap::Bind(int index) const
    {
        if (!m_ID) return;
        glBindTextureUnit(index, m_ID);
    }

    void OpenGLTextureCubeMap::Unbind() const
    {
        
    }
}