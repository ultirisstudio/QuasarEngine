#include "qepch.h"

#include "OpenGLTexture2D.h"
#include "OpenGLTextureUtils.h"

#include <stb_image.h>

#include <QuasarEngine/File/FileUtils.h>
#include <QuasarEngine/Core/Logger.h>

namespace QuasarEngine
{
	OpenGLTexture2D::OpenGLTexture2D(const TextureSpecification& specification) : Texture2D(specification) {}

	OpenGLTexture2D::~OpenGLTexture2D()
	{
		if (m_ID) glDeleteTextures(1, &m_ID);
	}

    bool OpenGLTexture2D::LoadFromPath(const std::string& path) {
        auto bytes = FileUtils::ReadFileBinary(path);
        if (bytes.empty()) {
            Q_ERROR("OpenGLTexture2D: failed to read file: " + path);
            return false;
        }
        return LoadFromMemory(ByteView{ bytes.data(), bytes.size() });
    }

    bool OpenGLTexture2D::LoadFromMemory(ByteView data) {
        if (data.empty()) {
            Q_ERROR("OpenGLTexture2D: empty memory buffer");
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

        int w = 0, h = 0, actualChannels = 0;
        const int desired = Utils::DesiredChannels(m_Specification.internal_format);
        unsigned char* decoded = stbi_load_from_memory(
            reinterpret_cast<const stbi_uc*>(data.data),
            static_cast<int>(data.size),
            &w, &h, &actualChannels, desired
        );

        if (!decoded) {
            Q_ERROR(std::string("stb_image decode failed: ") + stbi_failure_reason());
            return false;
        }

        m_Specification.width = static_cast<uint32_t>(w);
        m_Specification.height = static_cast<uint32_t>(h);
        m_Specification.channels = static_cast<uint32_t>(desired ? desired : actualChannels);

        const bool ok = LoadFromData(ByteView{ decoded, static_cast<std::size_t>(w * h * m_Specification.channels) });
        stbi_image_free(decoded);
        return ok;
    }

    bool OpenGLTexture2D::LoadFromData(ByteView pixels) {
        if (pixels.empty()) { Q_ERROR("OpenGLTexture2D: no pixel data"); return false; }
        if (m_Specification.width == 0 || m_Specification.height == 0) {
            Q_ERROR("OpenGLTexture2D: width/height must be set before LoadFromData()");
            return false;
        }
        return UploadPixelsDSA(pixels);
    }

    bool OpenGLTexture2D::UploadPixelsDSA(ByteView pixels) {
        const GLenum target = Utils::TargetFromSamples(m_Specification.Samples);
        const auto   glInt = Utils::ToGLFormat(m_Specification.internal_format);
        const auto   glExt = Utils::ToGLFormat(m_Specification.format);

        if (glInt.internal == 0 || glExt.external == 0) {
            Q_ERROR("OpenGLTexture2D: unsupported texture format mapping");
            return false;
        }

        if (m_ID) { glDeleteTextures(1, &m_ID); m_ID = 0; m_Loaded = false; }

        glCreateTextures(target, 1, &m_ID);

        if (target == GL_TEXTURE_2D) {
            glTextureParameteri(m_ID, GL_TEXTURE_WRAP_S, Utils::ToGLWrap(m_Specification.wrap_s));
            glTextureParameteri(m_ID, GL_TEXTURE_WRAP_T, Utils::ToGLWrap(m_Specification.wrap_t));
            glTextureParameteri(m_ID, GL_TEXTURE_MIN_FILTER, Utils::ToGLFilter(m_Specification.min_filter_param));
            glTextureParameteri(m_ID, GL_TEXTURE_MAG_FILTER, Utils::ToGLFilter(m_Specification.mag_filter_param));

            if (glInt.channels == 1) {
                const GLint swizzle[4] = { GL_RED, GL_RED, GL_RED, GL_ONE };
                glTextureParameteriv(m_ID, GL_TEXTURE_SWIZZLE_RGBA, swizzle);
            }

            const GLint levels = Utils::CalcMipmapLevels(static_cast<GLint>(m_Specification.width),
                static_cast<GLint>(m_Specification.height),
                m_Specification.mipmap);

            glTextureStorage2D(m_ID, levels, glInt.internal,
                static_cast<GLint>(m_Specification.width),
                static_cast<GLint>(m_Specification.height));

            GLint oldUnpack = 4;
            glGetIntegerv(GL_UNPACK_ALIGNMENT, &oldUnpack);
            glPixelStorei(GL_UNPACK_ALIGNMENT, 1);

            glTextureSubImage2D(m_ID, 0, 0, 0,
                static_cast<GLint>(m_Specification.width),
                static_cast<GLint>(m_Specification.height),
                glExt.external, glExt.type, pixels.data);

            glPixelStorei(GL_UNPACK_ALIGNMENT, oldUnpack);

            if (m_Specification.mipmap && levels > 1) {
                glGenerateTextureMipmap(m_ID);
            }
        }
        else {
            glTextureStorage2DMultisample(m_ID,
                static_cast<GLint>(m_Specification.Samples),
                glInt.internal,
                static_cast<GLint>(m_Specification.width),
                static_cast<GLint>(m_Specification.height),
                GL_FALSE);
        }

        m_Loaded = true;
        return true;
    }

    void OpenGLTexture2D::Bind(int index) const {
        if (!m_ID) return;
        glBindTextureUnit(index, m_ID);
    }

    void OpenGLTexture2D::Unbind() const {
        //glBindTextureUnit(0, 0);
    }
}
