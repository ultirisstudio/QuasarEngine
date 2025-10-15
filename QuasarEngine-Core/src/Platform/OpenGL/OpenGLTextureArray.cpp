#include "qepch.h"

#include "OpenGLTextureArray.h"
#include "OpenGLTextureUtils.h"

#include <QuasarEngine/File/FileUtils.h>
#include <QuasarEngine/Core/Logger.h>

namespace QuasarEngine
{
    OpenGLTextureArray::OpenGLTextureArray(const TextureSpecification& specification)
        : TextureArray(specification) {
    }

    OpenGLTextureArray::~OpenGLTextureArray() {
        if (m_ID) glDeleteTextures(1, &m_ID);
    }

    bool OpenGLTextureArray::LoadFromPath(const std::string& path) {
        auto bytes = FileUtils::ReadFileBinary(path);
        if (bytes.empty()) {
            Q_ERROR("OpenGLTextureArray: failed to read file: " + path);
            return false;
        }
        return LoadFromMemory(ByteView{ bytes.data(), bytes.size() });
    }

    bool OpenGLTextureArray::LoadFromMemory(ByteView data) {
        return LoadFromData(data);
    }

    bool OpenGLTextureArray::LoadFromData(ByteView pixels) {
        if (pixels.empty()) {
            Q_ERROR("OpenGLTextureArray: no pixel data");
            return false;
        }
        if (m_Specification.width == 0 || m_Specification.height == 0) {
            Q_ERROR("OpenGLTextureArray: width/height must be set before LoadFromData()");
            return false;
        }

        const auto glInt = ToGLFormat(m_Specification.internal_format);
        const auto glExt = ToGLFormat(m_Specification.format);
        const GLint channels = Utils::DesiredChannels(m_Specification.internal_format);
        if (glInt.internal == 0 || glExt.external == 0 || channels == 0) {
            Q_ERROR("OpenGLTextureArray: unsupported texture format mapping");
            return false;
        }

        const std::size_t texelCountPerLayer = static_cast<std::size_t>(m_Specification.width) *
            static_cast<std::size_t>(m_Specification.height) *
            static_cast<std::size_t>(channels);
        if (texelCountPerLayer == 0) {
            Q_ERROR("OpenGLTextureArray: invalid texel count per layer");
            return false;
        }
        if (pixels.size % texelCountPerLayer != 0) {
            Q_ERROR("OpenGLTextureArray: data size doesn't match (width*height*channels*layers)");
            return false;
        }

        const GLsizei layers = static_cast<GLsizei>(pixels.size / texelCountPerLayer);
        m_Specification.channels = static_cast<uint32_t>(channels);

        return UploadPixelsDSA(pixels, layers);
    }

    bool OpenGLTextureArray::UploadPixelsDSA(ByteView pixels, GLsizei layers) {
        const GLenum target = Utils::TargetFromSamples(m_Specification.Samples);
        const auto   glInt = ToGLFormat(m_Specification.internal_format);
        const auto   glExt = ToGLFormat(m_Specification.format);

        if (glInt.internal == 0 || glExt.external == 0) {
            Q_ERROR("OpenGLTextureArray: unsupported texture format mapping");
            return false;
        }

        if (m_ID) { glDeleteTextures(1, &m_ID); m_ID = 0; m_Loaded = false; }
        glCreateTextures(target, 1, &m_ID);

        if (target == GL_TEXTURE_2D_ARRAY) {
            glTextureParameteri(m_ID, GL_TEXTURE_WRAP_S, Utils::ToGLWrap(m_Specification.wrap_s));
            glTextureParameteri(m_ID, GL_TEXTURE_WRAP_T, Utils::ToGLWrap(m_Specification.wrap_t));
            glTextureParameteri(m_ID, GL_TEXTURE_WRAP_R, Utils::ToGLWrap(m_Specification.wrap_r));
            glTextureParameteri(m_ID, GL_TEXTURE_MIN_FILTER, Utils::ToGLFilter(m_Specification.min_filter_param));
            glTextureParameteri(m_ID, GL_TEXTURE_MAG_FILTER, Utils::ToGLFilter(m_Specification.mag_filter_param));

            if (glInt.channels == 1) {
                const GLint swizzle[4] = { GL_RED, GL_RED, GL_RED, GL_ONE };
                glTextureParameteriv(m_ID, GL_TEXTURE_SWIZZLE_RGBA, swizzle);
            }

            const GLint levels = Utils::CalcMipmapLevels(
                static_cast<GLint>(m_Specification.width),
                static_cast<GLint>(m_Specification.height),
                m_Specification.mipmap
            );

            glTextureStorage3D(m_ID, levels, glInt.internal,
                static_cast<GLint>(m_Specification.width),
                static_cast<GLint>(m_Specification.height),
                layers);

            GLint oldUnpack = 4;
            glGetIntegerv(GL_UNPACK_ALIGNMENT, &oldUnpack);
            glPixelStorei(GL_UNPACK_ALIGNMENT, 1);

            glTextureSubImage3D(m_ID, 0,
                0, 0, 0,
                static_cast<GLint>(m_Specification.width),
                static_cast<GLint>(m_Specification.height),
                layers,
                glExt.external, glExt.type, pixels.data);

            glPixelStorei(GL_UNPACK_ALIGNMENT, oldUnpack);

            if (m_Specification.mipmap && levels > 1) {
                glGenerateTextureMipmap(m_ID);
            }

            m_Layers = layers;
            m_Loaded = true;
            return true;
        }
        else {
            glTextureStorage3DMultisample(m_ID,
                static_cast<GLint>(m_Specification.Samples),
                glInt.internal,
                static_cast<GLint>(m_Specification.width),
                static_cast<GLint>(m_Specification.height),
                layers,
                GL_FALSE);
            glTextureParameteri(m_ID, GL_TEXTURE_WRAP_S, Utils::ToGLWrap(m_Specification.wrap_s));
            glTextureParameteri(m_ID, GL_TEXTURE_WRAP_T, Utils::ToGLWrap(m_Specification.wrap_t));
            glTextureParameteri(m_ID, GL_TEXTURE_WRAP_R, Utils::ToGLWrap(m_Specification.wrap_r));
            glTextureParameteri(m_ID, GL_TEXTURE_MIN_FILTER, Utils::ToGLFilter(m_Specification.min_filter_param));
            glTextureParameteri(m_ID, GL_TEXTURE_MAG_FILTER, Utils::ToGLFilter(m_Specification.mag_filter_param));

            m_Layers = layers;
            m_Loaded = true;
            return true;
        }
    }

    void OpenGLTextureArray::Bind(int index) const {
        if (!m_ID) return;
        glBindTextureUnit(index, m_ID);
    }

    void OpenGLTextureArray::Unbind() const {
        //glBindTextureUnit(0, 0);
    }
}