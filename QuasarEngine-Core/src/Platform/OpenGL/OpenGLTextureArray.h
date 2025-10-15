#pragma once

#include <QuasarEngine/Resources/TextureArray.h>
#include <glad/glad.h>

namespace QuasarEngine
{
    class OpenGLTextureArray : public TextureArray {
    public:
        explicit OpenGLTextureArray(const TextureSpecification& specification);
        ~OpenGLTextureArray() override;

        TextureHandle GetHandle() const noexcept override { return static_cast<TextureHandle>(m_ID); }
        bool IsLoaded() const noexcept override { return m_Loaded; }

        bool LoadFromPath(const std::string& path) override;
        bool LoadFromMemory(ByteView data) override;
        bool LoadFromData(ByteView data) override;

        void Bind(int index = 0) const override;
        void Unbind() const override;

    private:
        struct GLFormat {
            GLenum internal = 0;
            GLenum external = 0;
            GLenum type = GL_UNSIGNED_BYTE;
            GLint  channels = 0;
        };

        static GLFormat ToGLFormat(TextureFormat fmt);
        static GLenum   ToGLWrap(TextureWrap wrap);
        static GLenum   ToGLFilter(TextureFilter f);
        static GLint    DesiredChannels(TextureFormat fmt);
        static GLint    CalcMipmapLevels(GLint w, GLint h, bool mipmap);
        static GLenum   TargetFromSamples(uint32_t samples) {
            return samples > 1 ? GL_TEXTURE_2D_MULTISAMPLE_ARRAY : GL_TEXTURE_2D_ARRAY;
        }

        bool UploadPixelsDSA(ByteView pixels, GLsizei layers);

    private:
        GLuint m_ID = 0;
        bool   m_Loaded = false;
        GLsizei m_Layers = 0;
    };
}