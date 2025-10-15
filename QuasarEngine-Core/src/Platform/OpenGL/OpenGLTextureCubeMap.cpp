#include "qepch.h"

#include "OpenGLTextureCubeMap.h"
#include "OpenGLTextureUtils.h"

#include <stb_image.h>
#include <QuasarEngine/File/FileUtils.h>
#include <QuasarEngine/Core/Logger.h>

namespace QuasarEngine
{
    OpenGLTextureCubeMap::OpenGLTextureCubeMap(const TextureSpecification& specification)
        : TextureCubeMap(specification) {
    }

    OpenGLTextureCubeMap::~OpenGLTextureCubeMap() {
        if (m_ID) glDeleteTextures(1, &m_ID);
    }

    bool OpenGLTextureCubeMap::AllocateStorageIfNeeded(uint32_t w, uint32_t h) {
        if (m_StorageAllocated) return true;

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

        const GLint levels = Utils::CalcMipmapLevels(static_cast<GLint>(w),
            static_cast<GLint>(h),
            m_Specification.mipmap);
        glTextureStorage2D(m_ID, levels, glInt.internal, static_cast<GLint>(w), static_cast<GLint>(h));

        m_StorageAllocated = true;
        m_FacesUploaded = 0;
        return true;
    }

    bool OpenGLTextureCubeMap::UploadAllFacesDSA(ByteView allFacesPixels) {
        const auto glExt = Utils::ToGLFormat(m_Specification.format);
        const auto glInt = Utils::ToGLFormat(m_Specification.internal_format);
        if (glExt.external == 0 || glInt.internal == 0) {
            Q_ERROR("OpenGLTextureCubeMap: unsupported format mapping");
            return false;
        }

        GLint oldUnpack = 4;
        glGetIntegerv(GL_UNPACK_ALIGNMENT, &oldUnpack);
        glPixelStorei(GL_UNPACK_ALIGNMENT, 1);

        glTextureSubImage3D(
            m_ID, 0,
            0, 0, 0,
            static_cast<GLint>(m_Specification.width),
            static_cast<GLint>(m_Specification.height),
            6,
            glExt.external, glExt.type,
            allFacesPixels.data
        );

        glPixelStorei(GL_UNPACK_ALIGNMENT, oldUnpack);

        if (m_Specification.mipmap) {
            glGenerateTextureMipmap(m_ID);
        }

        m_FacesUploaded = 6;
        m_Loaded = true;
        return true;
    }

    bool OpenGLTextureCubeMap::UploadFaceDSA(Face face, ByteView facePixels, uint32_t w, uint32_t h) {
        const auto glExt = Utils::ToGLFormat(m_Specification.format);
        if (glExt.external == 0) {
            Q_ERROR("OpenGLTextureCubeMap: unsupported external format");
            return false;
        }

        GLint oldUnpack = 4;
        glGetIntegerv(GL_UNPACK_ALIGNMENT, &oldUnpack);
        glPixelStorei(GL_UNPACK_ALIGNMENT, 1);

        glTextureSubImage3D(
            m_ID, 0,
            0, 0, FaceIndex(face),
            static_cast<GLint>(w),
            static_cast<GLint>(h),
            1,
            glExt.external, glExt.type,
            facePixels.data
        );

        glPixelStorei(GL_UNPACK_ALIGNMENT, oldUnpack);

        if (++m_FacesUploaded == 6 && m_Specification.mipmap) {
            glGenerateTextureMipmap(m_ID);
        }

        if (m_FacesUploaded == 6) m_Loaded = true;
        return true;
    }

    bool OpenGLTextureCubeMap::LoadFromPath(const std::string& path) {
        auto bytes = FileUtils::ReadFileBinary(path);
        if (bytes.empty()) {
            Q_ERROR("OpenGLTextureCubeMap: failed to read file: " + path);
            return false;
        }
        return LoadFromMemory(ByteView{ bytes.data(), bytes.size() });
    }

    bool OpenGLTextureCubeMap::LoadFromMemory(ByteView data) {
        if (data.empty()) {
            Q_ERROR("OpenGLTextureCubeMap: empty memory buffer");
            return false;
        }

        if (m_Specification.flip)  stbi_set_flip_vertically_on_load(true);
        else                       stbi_set_flip_vertically_on_load(false);

        if (m_Specification.alpha) {
            m_Specification.format = TextureFormat::RGBA;
            m_Specification.internal_format = m_Specification.gamma ? TextureFormat::SRGB8A8 : TextureFormat::RGBA8;
        }
        else {
            m_Specification.format = TextureFormat::RGB;
            m_Specification.internal_format = m_Specification.gamma ? TextureFormat::SRGB8 : TextureFormat::RGB8;
        }

        int w = 0, h = 0, actual = 0;
        const int desired = Utils::DesiredChannels(m_Specification.internal_format);
        unsigned char* decoded = stbi_load_from_memory(
            reinterpret_cast<const stbi_uc*>(data.data),
            static_cast<int>(data.size),
            &w, &h, &actual, desired
        );
        if (!decoded) {
            Q_ERROR(std::string("OpenGLTextureCubeMap: stb_image decode failed: ") + stbi_failure_reason());
            return false;
        }

        m_Specification.width = static_cast<uint32_t>(w);
        m_Specification.height = static_cast<uint32_t>(h);
        m_Specification.channels = static_cast<uint32_t>(desired ? desired : actual);

        if (!AllocateStorageIfNeeded(m_Specification.width, m_Specification.height)) {
            stbi_image_free(decoded);
            return false;
        }

        bool ok = true;
        for (int fi = 0; fi < 6; ++fi) {
            ok &= UploadFaceDSA(static_cast<Face>(fi),
                ByteView{ decoded, static_cast<std::size_t>(w * h * m_Specification.channels) },
                m_Specification.width, m_Specification.height);
        }
        stbi_image_free(decoded);
        return ok;
    }

    bool OpenGLTextureCubeMap::LoadFromData(ByteView data) {
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

        const std::size_t faceSize = static_cast<std::size_t>(m_Specification.width) *
            static_cast<std::size_t>(m_Specification.height) *
            static_cast<std::size_t>(channels);

        if (!AllocateStorageIfNeeded(m_Specification.width, m_Specification.height))
            return false;

        if (data.size == faceSize * 6) {
            return UploadAllFacesDSA(data);
        }
        else if (data.size == faceSize) {
            bool ok = true;
            for (int fi = 0; fi < 6; ++fi) {
                ok &= UploadFaceDSA(static_cast<Face>(fi), data, m_Specification.width, m_Specification.height);
            }
            return ok;
        }
        else {
            Q_ERROR("OpenGLTextureCubeMap: data size must be faceSize or 6*faceSize");
            return false;
        }
    }

    bool OpenGLTextureCubeMap::LoadFaceFromPath(Face face, const std::string& path) {
        auto bytes = FileUtils::ReadFileBinary(path);
        if (bytes.empty()) {
            Q_ERROR("OpenGLTextureCubeMap: failed to read face file: " + path);
            return false;
        }
        return LoadFaceFromMemory(face, ByteView{ bytes.data(), bytes.size() });
    }

    bool OpenGLTextureCubeMap::LoadFaceFromMemory(Face face, ByteView data) {
        if (data.empty()) {
            Q_ERROR("OpenGLTextureCubeMap: empty memory buffer (face)");
            return false;
        }

        if (m_Specification.flip)  stbi_set_flip_vertically_on_load(true);
        else                       stbi_set_flip_vertically_on_load(false);

        if (m_Specification.alpha) {
            m_Specification.format = TextureFormat::RGBA;
            m_Specification.internal_format = m_Specification.gamma ? TextureFormat::SRGB8A8 : TextureFormat::RGBA8;
        }
        else {
            m_Specification.format = TextureFormat::RGB;
            m_Specification.internal_format = m_Specification.gamma ? TextureFormat::SRGB8 : TextureFormat::RGB8;
        }

        int w = 0, h = 0, actual = 0;
        const int desired = Utils::DesiredChannels(m_Specification.internal_format);
        unsigned char* decoded = stbi_load_from_memory(
            reinterpret_cast<const stbi_uc*>(data.data),
            static_cast<int>(data.size),
            &w, &h, &actual, desired
        );
        if (!decoded) {
            Q_ERROR(std::string("OpenGLTextureCubeMap: stb_image face decode failed: ") + stbi_failure_reason());
            return false;
        }

        if (!m_StorageAllocated) {
            m_Specification.width = static_cast<uint32_t>(w);
            m_Specification.height = static_cast<uint32_t>(h);
            m_Specification.channels = static_cast<uint32_t>(desired ? desired : actual);
            if (!AllocateStorageIfNeeded(m_Specification.width, m_Specification.height)) {
                stbi_image_free(decoded);
                return false;
            }
        }
        else {
            if (m_Specification.width != static_cast<uint32_t>(w) ||
                m_Specification.height != static_cast<uint32_t>(h)) {
                Q_ERROR("OpenGLTextureCubeMap: face dimensions mismatch with allocated storage");
                stbi_image_free(decoded);
                return false;
            }
        }

        const bool ok = UploadFaceDSA(face,
            ByteView{ decoded, static_cast<std::size_t>(w * h * (desired ? desired : actual)) },
            m_Specification.width, m_Specification.height);
        stbi_image_free(decoded);
        return ok;
    }

    bool OpenGLTextureCubeMap::LoadFaceFromData(Face face, ByteView data, uint32_t w, uint32_t h, uint32_t channels) {
        if (data.empty() || w == 0 || h == 0 || channels == 0) {
            Q_ERROR("OpenGLTextureCubeMap: invalid face data");
            return false;
        }

        const GLint expected = Utils::DesiredChannels(m_Specification.internal_format);
        if (expected != 0 && static_cast<GLint>(channels) != expected) {
            Q_ERROR("OpenGLTextureCubeMap: channels mismatch with internal format");
            return false;
        }

        if (!m_StorageAllocated) {
            m_Specification.width = w;
            m_Specification.height = h;
            m_Specification.channels = channels;
            if (!AllocateStorageIfNeeded(w, h)) return false;
        }
        else {
            if (m_Specification.width != w || m_Specification.height != h) {
                Q_ERROR("OpenGLTextureCubeMap: face dimensions mismatch with allocated storage");
                return false;
            }
        }

        return UploadFaceDSA(face, data, w, h);
    }

    void OpenGLTextureCubeMap::Bind(int index) const {
        if (!m_ID) return;
        glBindTextureUnit(index, m_ID);
    }

    void OpenGLTextureCubeMap::Unbind() const {
        //glBindTextureUnit(0, 0);
    }
}
