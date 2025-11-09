#include "qepch.h"

#include "OpenGLTextureArray.h"
#include "OpenGLTextureUtils.h"

#include <QuasarEngine/File/FileUtils.h>
#include <QuasarEngine/Core/Logger.h>

#include <stb_image.h>
#include <glad/glad.h>
#include <algorithm>
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

    static void ConfigureTextureFixedState(GLuint tex, const TextureSpecification& spec,
        const Utils::GLFormat& glInt, uint32_t levels, bool isArray)
    {
        glTextureParameteri(tex, GL_TEXTURE_WRAP_S, Utils::ToGLWrap(spec.wrap_s));
        glTextureParameteri(tex, GL_TEXTURE_WRAP_T, Utils::ToGLWrap(spec.wrap_t));
        glTextureParameteri(tex, GL_TEXTURE_WRAP_R, Utils::ToGLWrap(spec.wrap_r));
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
        glSamplerParameteri(sampler, GL_TEXTURE_WRAP_R, Utils::ToGLWrap(spec.wrap_r));
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

    OpenGLTextureArray::OpenGLTextureArray(const TextureSpecification& specification)
        : TextureArray(specification)
    {
    }

    OpenGLTextureArray::~OpenGLTextureArray()
    {
        if (m_ID) glDeleteTextures(1, &m_ID);
        if (m_SamplerID) { glDeleteSamplers(1, &m_SamplerID); m_SamplerID = 0; }
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

        m_Specification.width = width;
        m_Specification.height = height;

        if (target == GL_TEXTURE_2D_ARRAY)
        {
            const uint32_t levels = Utils::CalcMipLevelsFromSpec(m_Specification);
            glTextureStorage3D(m_ID, (GLint)levels, glInt.internal, (GLint)width, (GLint)height, layers);
            ConfigureTextureFixedState(m_ID, m_Specification, glInt, levels, true);
        }
        else
        {
            glTextureStorage3DMultisample(m_ID, (GLint)m_Specification.Samples, glInt.internal, (GLint)width, (GLint)height, layers, GL_FALSE);
            ConfigureTextureFixedState(m_ID, m_Specification, glInt, 1, true);
        }

        ConfigureOrCreateSampler(m_SamplerID, m_Specification);
        m_Layers = layers;
        m_Loaded = true;
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
        GLint oldUnpack = 0;
        glGetIntegerv(GL_UNPACK_ALIGNMENT, &oldUnpack);
        glPixelStorei(GL_UNPACK_ALIGNMENT, 1);

        {
            int w = 0, h = 0, n = 0;
            if (wantFloat) {
                float* img = stbi_loadf(paths[0].c_str(), &w, &h, &n, desired > 0 ? desired : 0);
                if (!img) {
                    Q_ERROR(std::string("stb_image (float) failed: ") + stbi_failure_reason());
                    return false;
                }
                n0 = (desired > 0 ? desired : n); w0 = w; h0 = h;
                m_Specification.width = (uint32_t)w0;
                m_Specification.height = (uint32_t)h0;
                m_Specification.channels = (uint32_t)n0;
                if (!AllocateStorage(m_Specification.width, m_Specification.height, (GLsizei)paths.size())) { stbi_image_free(img); return false; }

                const auto glExt = Utils::ToGLFormat(m_Specification.format);
                if (m_Specification.async_upload) {
                    GLuint pbo = 0; glCreateBuffers(1, &pbo);
                    const size_t sz = (size_t)w0 * (size_t)h0 * (size_t)n0 * sizeof(float);
                    glNamedBufferData(pbo, (GLsizeiptr)sz, nullptr, GL_STREAM_DRAW);
                    void* dst = glMapNamedBufferRange(pbo, 0, (GLsizeiptr)sz, GL_MAP_WRITE_BIT | GL_MAP_INVALIDATE_BUFFER_BIT);
                    if (dst) { std::memcpy(dst, img, sz); glUnmapNamedBuffer(pbo); }
                    glBindBuffer(GL_PIXEL_UNPACK_BUFFER, pbo);
                    glTextureSubImage3D(m_ID, 0, 0, 0, 0, (GLint)w0, (GLint)h0, 1, glExt.external, GL_FLOAT, (const void*)0);
                    glBindBuffer(GL_PIXEL_UNPACK_BUFFER, 0);
                    glDeleteBuffers(1, &pbo);
                }
                else {
                    glTextureSubImage3D(m_ID, 0, 0, 0, 0, (GLint)w0, (GLint)h0, 1, glExt.external, GL_FLOAT, img);
                }
                stbi_image_free(img);
            }
            else {
                unsigned char* img = stbi_load(paths[0].c_str(), &w, &h, &n, desired > 0 ? desired : 0);
                if (!img) { Q_ERROR(std::string("stb_image failed: ") + stbi_failure_reason()); return false; }
                n0 = (desired > 0 ? desired : n); w0 = w; h0 = h;
                m_Specification.width = (uint32_t)w0;
                m_Specification.height = (uint32_t)h0;
                m_Specification.channels = (uint32_t)n0;
                if (!AllocateStorage(m_Specification.width, m_Specification.height, (GLsizei)paths.size())) { stbi_image_free(img); return false; }

                const auto glExt = Utils::ToGLFormat(m_Specification.format);
                if (m_Specification.async_upload) {
                    GLuint pbo = 0; glCreateBuffers(1, &pbo);
                    const size_t sz = (size_t)w0 * (size_t)h0 * (size_t)n0;
                    glNamedBufferData(pbo, (GLsizeiptr)sz, nullptr, GL_STREAM_DRAW);
                    void* dst = glMapNamedBufferRange(pbo, 0, (GLsizeiptr)sz, GL_MAP_WRITE_BIT | GL_MAP_INVALIDATE_BUFFER_BIT);
                    if (dst) { std::memcpy(dst, img, sz); glUnmapNamedBuffer(pbo); }
                    glBindBuffer(GL_PIXEL_UNPACK_BUFFER, pbo);
                    glTextureSubImage3D(m_ID, 0, 0, 0, 0, (GLint)w0, (GLint)h0, 1, glExt.external, glExt.type, (const void*)0);
                    glBindBuffer(GL_PIXEL_UNPACK_BUFFER, 0);
                    glDeleteBuffers(1, &pbo);
                }
                else {
                    glTextureSubImage3D(m_ID, 0, 0, 0, 0, (GLint)w0, (GLint)h0, 1, glExt.external, glExt.type, img);
                }
                stbi_image_free(img);
            }
        }

        const auto glExt = Utils::ToGLFormat(m_Specification.format);
        for (GLint layer = 1; layer < (GLint)paths.size(); ++layer) {
            int w = 0, h = 0, n = 0;
            if (wantFloat) {
                float* img = stbi_loadf(paths[layer].c_str(), &w, &h, &n, desired > 0 ? desired : 0);
                if (!img) { Q_ERROR(std::string("stb_image (float) failed: ") + stbi_failure_reason()); return false; }
                if (w != w0 || h != h0 || (desired > 0 ? desired : n) != n0) { stbi_image_free(img); Q_ERROR("OpenGLTextureArray::LoadFromFiles: all layers must have identical size & channels"); return false; }
                if (m_Specification.async_upload) {
                    GLuint pbo = 0; glCreateBuffers(1, &pbo);
                    const size_t sz = (size_t)w0 * (size_t)h0 * (size_t)n0 * sizeof(float);
                    glNamedBufferData(pbo, (GLsizeiptr)sz, nullptr, GL_STREAM_DRAW);
                    void* dst = glMapNamedBufferRange(pbo, 0, (GLsizeiptr)sz, GL_MAP_WRITE_BIT | GL_MAP_INVALIDATE_BUFFER_BIT);
                    if (dst) { std::memcpy(dst, img, sz); glUnmapNamedBuffer(pbo); }
                    glBindBuffer(GL_PIXEL_UNPACK_BUFFER, pbo);
                    glTextureSubImage3D(m_ID, 0, 0, 0, layer, (GLint)w0, (GLint)h0, 1, glExt.external, GL_FLOAT, (const void*)0);
                    glBindBuffer(GL_PIXEL_UNPACK_BUFFER, 0);
                    glDeleteBuffers(1, &pbo);
                }
                else {
                    glTextureSubImage3D(m_ID, 0, 0, 0, layer, (GLint)w0, (GLint)h0, 1, glExt.external, GL_FLOAT, img);
                }
                stbi_image_free(img);
            }
            else {
                unsigned char* img = stbi_load(paths[layer].c_str(), &w, &h, &n, desired > 0 ? desired : 0);
                if (!img) { Q_ERROR(std::string("stb_image failed: ") + stbi_failure_reason()); return false; }
                if (w != w0 || h != h0 || (desired > 0 ? desired : n) != n0) { stbi_image_free(img); Q_ERROR("OpenGLTextureArray::LoadFromFiles: all layers must have identical size & channels"); return false; }
                if (m_Specification.async_upload) {
                    GLuint pbo = 0; glCreateBuffers(1, &pbo);
                    const size_t sz = (size_t)w0 * (size_t)h0 * (size_t)n0;
                    glNamedBufferData(pbo, (GLsizeiptr)sz, nullptr, GL_STREAM_DRAW);
                    void* dst = glMapNamedBufferRange(pbo, 0, (GLsizeiptr)sz, GL_MAP_WRITE_BIT | GL_MAP_INVALIDATE_BUFFER_BIT);
                    if (dst) { std::memcpy(dst, img, sz); glUnmapNamedBuffer(pbo); }
                    glBindBuffer(GL_PIXEL_UNPACK_BUFFER, pbo);
                    glTextureSubImage3D(m_ID, 0, 0, 0, layer, (GLint)w0, (GLint)h0, 1, glExt.external, glExt.type, (const void*)0);
                    glBindBuffer(GL_PIXEL_UNPACK_BUFFER, 0);
                    glDeleteBuffers(1, &pbo);
                }
                else {
                    glTextureSubImage3D(m_ID, 0, 0, 0, layer, (GLint)w0, (GLint)h0, 1, glExt.external, glExt.type, img);
                }
                stbi_image_free(img);
            }
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

        if (target == GL_TEXTURE_2D_ARRAY)
        {
            const uint32_t levels = Utils::CalcMipLevelsFromSpec(m_Specification);
            glTextureStorage3D(m_ID, (GLint)levels, glInt.internal,
                (GLint)m_Specification.width, (GLint)m_Specification.height, layers);
            ConfigureTextureFixedState(m_ID, m_Specification, glInt, levels, true);

            GLint oldUnpack = 4;
            glGetIntegerv(GL_UNPACK_ALIGNMENT, &oldUnpack);
            glPixelStorei(GL_UNPACK_ALIGNMENT, 1);

            const GLenum uploadType = pixelsAreFloat ? GL_FLOAT : glExt.type;

            if (m_Specification.async_upload) {
                GLuint pbo = 0;
                glCreateBuffers(1, &pbo);
                glNamedBufferData(pbo, (GLsizeiptr)pixels.size, nullptr, GL_STREAM_DRAW);
                void* dst = glMapNamedBufferRange(pbo, 0, (GLsizeiptr)pixels.size,
                    GL_MAP_WRITE_BIT | GL_MAP_INVALIDATE_BUFFER_BIT);
                if (dst) { std::memcpy(dst, pixels.data, pixels.size); glUnmapNamedBuffer(pbo); }
                glBindBuffer(GL_PIXEL_UNPACK_BUFFER, pbo);
                glTextureSubImage3D(m_ID, 0, 0, 0, 0,
                    (GLint)m_Specification.width, (GLint)m_Specification.height, layers,
                    glExt.external, uploadType, (const void*)0);
                glBindBuffer(GL_PIXEL_UNPACK_BUFFER, 0);
                glDeleteBuffers(1, &pbo);
            }
            else {
                glTextureSubImage3D(m_ID, 0, 0, 0, 0,
                    (GLint)m_Specification.width, (GLint)m_Specification.height, layers,
                    glExt.external, uploadType, pixels.data);
            }

            glPixelStorei(GL_UNPACK_ALIGNMENT, oldUnpack);

            if (m_Specification.auto_generate_mips && levels > 1) {
                glGenerateTextureMipmap(m_ID);
            }

            ConfigureOrCreateSampler(m_SamplerID, m_Specification);
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

            ConfigureTextureFixedState(m_ID, m_Specification, glInt, 1, true);
            ConfigureOrCreateSampler(m_SamplerID, m_Specification);
            m_Layers = layers;
            m_Loaded = true;
            return true;
        }
    }

    void OpenGLTextureArray::Bind(int index) const
    {
        if (!m_ID) return;
        glBindTextureUnit(index, m_ID);
        if (m_SamplerID) glBindSampler(index, m_SamplerID);
    }

    void OpenGLTextureArray::Unbind() const
    {
    }
}