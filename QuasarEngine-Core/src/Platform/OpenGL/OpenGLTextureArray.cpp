#include "qepch.h"

#include "OpenGLTextureArray.h"
#include "OpenGLTextureUtils.h"

#include <QuasarEngine/File/FileUtils.h>
#include <QuasarEngine/Core/Logger.h>

#include <stb_image.h>
#include <glad/glad.h>
#include <cmath>
#include <cstring>

namespace QuasarEngine
{
    namespace Utils
    {
        static inline GLenum TargetArrayFromSamples(uint32_t samples)
        {
            return samples > 1 ? GL_TEXTURE_2D_MULTISAMPLE_ARRAY : GL_TEXTURE_2D_ARRAY;
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
    }

    OpenGLTextureArray::OpenGLTextureArray(const TextureSpecification& specification)
        : TextureArray(specification)
    {
    }

    OpenGLTextureArray::~OpenGLTextureArray()
    {
        if (m_ID) glDeleteTextures(1, &m_ID);
    }

    bool OpenGLTextureArray::AllocateStorage(uint32_t width, uint32_t height, GLsizei layers)
    {
        if (width == 0 || height == 0 || layers <= 0) {
            Q_ERROR("OpenGLTextureArray::AllocateStorage: invalid dimensions/layers");
            return false;
        }

        if (m_ID) { glDeleteTextures(1, &m_ID); m_ID = 0; m_Loaded = false; }

        const auto glInt = Utils::ToGLFormat(m_Specification.internal_format);
        if (glInt.internal == 0) {
            Q_ERROR("OpenGLTextureArray::AllocateStorage: invalid internal format");
            return false;
        }

        const GLenum target = Utils::TargetArrayFromSamples(m_Specification.Samples);
        glCreateTextures(target, 1, &m_ID);

        glTextureParameteri(m_ID, GL_TEXTURE_WRAP_S, Utils::ToGLWrap(m_Specification.wrap_s));
        glTextureParameteri(m_ID, GL_TEXTURE_WRAP_T, Utils::ToGLWrap(m_Specification.wrap_t));
        glTextureParameteri(m_ID, GL_TEXTURE_WRAP_R, Utils::ToGLWrap(m_Specification.wrap_r));
        glTextureParameteri(m_ID, GL_TEXTURE_MIN_FILTER, Utils::ToGLFilter(m_Specification.min_filter_param));
        glTextureParameteri(m_ID, GL_TEXTURE_MAG_FILTER, Utils::ToGLFilter(m_Specification.mag_filter_param));

        if (glInt.channels == 1) {
            const GLint swizzle[4] = { GL_RED, GL_RED, GL_RED, GL_ONE };
            glTextureParameteriv(m_ID, GL_TEXTURE_SWIZZLE_RGBA, swizzle);
        }

        m_Specification.width = width;
        m_Specification.height = height;

        if (target == GL_TEXTURE_2D_ARRAY)
        {
            const uint32_t levels = Utils::CalcMipLevelsFromSpec(m_Specification);
            glTextureStorage3D(m_ID, (GLint)levels, glInt.internal,
                (GLint)width, (GLint)height, layers);
        }
        else
        {
            glTextureStorage3DMultisample(m_ID,
                (GLint)m_Specification.Samples,
                glInt.internal,
                (GLint)width, (GLint)height, layers,
                GL_FALSE);
        }

        m_Layers = layers;

        return true;
    }

    void OpenGLTextureArray::GenerateMips()
    {
        if (!m_ID) return;
        if (m_Specification.Samples > 1) return;
        glGenerateTextureMipmap(m_ID);
    }

    bool OpenGLTextureArray::LoadFromPath(const std::string& path)
    {
        auto bytes = FileUtils::ReadFileBinary(path);
        if (bytes.empty()) {
            Q_ERROR("OpenGLTextureArray: failed to read file: " + path);
            return false;
        }
        return LoadFromMemory(ByteView{ bytes.data(), bytes.size() });
    }

    bool OpenGLTextureArray::LoadFromMemory(ByteView data)
    {
        return LoadFromData(data);
    }

    bool OpenGLTextureArray::LoadFromData(ByteView pixels)
    {
        if (pixels.empty()) {
            Q_ERROR("OpenGLTextureArray: no pixel data");
            return false;
        }
        if (m_Specification.width == 0 || m_Specification.height == 0) {
            Q_ERROR("OpenGLTextureArray: width/height must be set before LoadFromData()");
            return false;
        }
        if (m_Specification.Samples > 1) {
            Q_ERROR("OpenGLTextureArray: cannot upload pixels to MULTISAMPLE array textures");
            return false;
        }

        const auto glInt = Utils::ToGLFormat(m_Specification.internal_format);
        const auto glExt = Utils::ToGLFormat(m_Specification.format);
        const GLint channels = Utils::DesiredChannels(m_Specification.internal_format);
        if (glInt.internal == 0 || glExt.external == 0 || channels == 0) {
            Q_ERROR("OpenGLTextureArray: unsupported texture format mapping");
            return false;
        }

        const bool pixelsAreFloat = Utils::IsFloatInternal(m_Specification.internal_format);
        const size_t bytesPerTexel = (size_t)channels * (pixelsAreFloat ? sizeof(float) : sizeof(uint8_t));
        const size_t layerBytes = (size_t)m_Specification.width * (size_t)m_Specification.height * bytesPerTexel;

        if (layerBytes == 0 || (pixels.size % layerBytes) != 0) {
            Q_ERROR("OpenGLTextureArray: data size doesn't match width*height*channels*(bytes)*layers");
            return false;
        }

        const GLsizei layers = (GLsizei)(pixels.size / layerBytes);
        m_Specification.channels = (uint32_t)channels;

        return UploadPixelsDSA(pixels, layers, pixelsAreFloat);
    }

    bool OpenGLTextureArray::LoadFromFiles(const std::vector<std::string>& paths)
    {
        if (paths.empty()) {
            Q_ERROR("OpenGLTextureArray::LoadFromFiles: empty path list");
            return false;
        }
        if (m_Specification.Samples > 1) {
            Q_ERROR("OpenGLTextureArray: cannot upload pixels to MULTISAMPLE array textures");
            return false;
        }

        stbi_set_flip_vertically_on_load(m_Specification.flip);

        const bool wantFloat = Utils::IsFloatInternal(m_Specification.internal_format);
        const int desired = Utils::DesiredChannels(m_Specification.internal_format);
        int w0 = -1, h0 = -1, n0 = -1;

        std::vector<std::vector<uint8_t>> layersData;
        layersData.reserve(paths.size());

        for (const auto& p : paths)
        {
            int w = 0, h = 0, n = 0;

            if (wantFloat) {
                float* img = stbi_loadf(p.c_str(), &w, &h, &n, desired > 0 ? desired : 0);
                if (!img) {
                    Q_ERROR(std::string("stb_image (float) failed: ") + stbi_failure_reason());
                    return false;
                }
                const int ch = desired > 0 ? desired : n;
                const size_t bytes = (size_t)w * (size_t)h * (size_t)ch * sizeof(float);
                std::vector<uint8_t> layer(bytes);
                std::memcpy(layer.data(), img, bytes);
                stbi_image_free(img);
                layersData.push_back(std::move(layer));
            }
            else {
                unsigned char* img = stbi_load(p.c_str(), &w, &h, &n, desired > 0 ? desired : 0);
                if (!img) {
                    Q_ERROR(std::string("stb_image failed: ") + stbi_failure_reason());
                    return false;
                }
                const int ch = desired > 0 ? desired : n;
                const size_t bytes = (size_t)w * (size_t)h * (size_t)ch * sizeof(uint8_t);
                std::vector<uint8_t> layer(bytes);
                std::memcpy(layer.data(), img, bytes);
                stbi_image_free(img);
                layersData.push_back(std::move(layer));
            }

            if (w0 < 0) { w0 = w; h0 = h; n0 = (desired > 0 ? desired : n); }
            else if (w != w0 || h != h0 || (desired > 0 ? desired : n) != n0) {
                Q_ERROR("OpenGLTextureArray::LoadFromFiles: all layers must have identical size & channels");
                return false;
            }
        }

        m_Specification.width = (uint32_t)w0;
        m_Specification.height = (uint32_t)h0;
        m_Specification.channels = (uint32_t)n0;

        if (!AllocateStorage(m_Specification.width, m_Specification.height, (GLsizei)paths.size()))
            return false;

        const auto glExt = Utils::ToGLFormat(m_Specification.format);
        if (glExt.external == 0) {
            Q_ERROR("OpenGLTextureArray::LoadFromFiles: invalid external format");
            return false;
        }

        const GLenum target = GL_TEXTURE_2D_ARRAY;

        GLint oldUnpack = 4;
        glGetIntegerv(GL_UNPACK_ALIGNMENT, &oldUnpack);
        glPixelStorei(GL_UNPACK_ALIGNMENT, 1);

        const GLenum uploadType = wantFloat ? GL_FLOAT : glExt.type;

        for (GLint layer = 0; layer < (GLint)paths.size(); ++layer)
        {
            const auto& layerData = layersData[(size_t)layer];
            glTextureSubImage3D(m_ID, 0,
                0, 0, layer,
                (GLint)m_Specification.width, (GLint)m_Specification.height, 1,
                glExt.external, uploadType,
                layerData.data());
        }

        glPixelStorei(GL_UNPACK_ALIGNMENT, oldUnpack);

        if (m_Specification.auto_generate_mips) GenerateMips();

        m_Layers = (GLsizei)paths.size();
        m_Loaded = true;
        return true;
    }

    bool OpenGLTextureArray::UploadPixelsDSA(ByteView pixels, GLsizei layers, bool pixelsAreFloat)
    {
        const GLenum target = Utils::TargetArrayFromSamples(m_Specification.Samples);
        const auto   glInt = Utils::ToGLFormat(m_Specification.internal_format);
        const auto   glExt = Utils::ToGLFormat(m_Specification.format);

        if (glInt.internal == 0 || glExt.external == 0) {
            Q_ERROR("OpenGLTextureArray: unsupported texture format mapping");
            return false;
        }

        if (m_ID) { glDeleteTextures(1, &m_ID); m_ID = 0; m_Loaded = false; }
        glCreateTextures(target, 1, &m_ID);

        glTextureParameteri(m_ID, GL_TEXTURE_WRAP_S, Utils::ToGLWrap(m_Specification.wrap_s));
        glTextureParameteri(m_ID, GL_TEXTURE_WRAP_T, Utils::ToGLWrap(m_Specification.wrap_t));
        glTextureParameteri(m_ID, GL_TEXTURE_WRAP_R, Utils::ToGLWrap(m_Specification.wrap_r));
        glTextureParameteri(m_ID, GL_TEXTURE_MIN_FILTER, Utils::ToGLFilter(m_Specification.min_filter_param));
        glTextureParameteri(m_ID, GL_TEXTURE_MAG_FILTER, Utils::ToGLFilter(m_Specification.mag_filter_param));

        if (glInt.channels == 1) {
            const GLint swizzle[4] = { GL_RED, GL_RED, GL_RED, GL_ONE };
            glTextureParameteriv(m_ID, GL_TEXTURE_SWIZZLE_RGBA, swizzle);
        }

        if (target == GL_TEXTURE_2D_ARRAY)
        {
            const uint32_t levels = Utils::CalcMipLevelsFromSpec(m_Specification);
            glTextureStorage3D(m_ID, (GLint)levels, glInt.internal,
                (GLint)m_Specification.width,
                (GLint)m_Specification.height,
                layers);

            GLint oldUnpack = 4;
            glGetIntegerv(GL_UNPACK_ALIGNMENT, &oldUnpack);
            glPixelStorei(GL_UNPACK_ALIGNMENT, 1);

            const GLenum uploadType = pixelsAreFloat ? GL_FLOAT : glExt.type;

            glTextureSubImage3D(m_ID, 0,
                0, 0, 0,
                (GLint)m_Specification.width,
                (GLint)m_Specification.height,
                layers,
                glExt.external, uploadType, pixels.data);

            glPixelStorei(GL_UNPACK_ALIGNMENT, oldUnpack);

            if (m_Specification.auto_generate_mips && levels > 1) {
                glGenerateTextureMipmap(m_ID);
            }

            m_Layers = layers;
            m_Loaded = true;
            return true;
        }
        else
        {
            glTextureStorage3DMultisample(m_ID,
                (GLint)m_Specification.Samples, glInt.internal,
                (GLint)m_Specification.width, (GLint)m_Specification.height,
                layers, GL_FALSE);

            m_Layers = layers;
            m_Loaded = true;
            return true;
        }
    }

    void OpenGLTextureArray::Bind(int index) const
    {
        if (!m_ID) return;
        glBindTextureUnit(index, m_ID);
    }

    void OpenGLTextureArray::Unbind() const
    {
        
    }
}
