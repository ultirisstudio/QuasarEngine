#pragma once

#include <QuasarEngine/Resources/Texture.h>
#include <glad/glad.h>

namespace QuasarEngine
{
    class OpenGLTexture2DView : public Texture {
    public:
        OpenGLTexture2DView(GLuint id, GLenum target, const TextureSpecification& spec)
            : Texture(spec), m_ID(id), m_Target(target) {
        }

        TextureHandle GetHandle() const noexcept override { return static_cast<TextureHandle>(m_ID); }
        bool IsLoaded() const noexcept override { return true; }

        void Bind(int unit = 0) const override { glBindTextureUnit(unit, m_ID); }
        void Unbind() const override {}

        bool LoadFromPath(const std::string&) override { return false; }
        bool LoadFromMemory(ByteView) override { return false; }
        bool LoadFromData(ByteView) override { return false; }

    private:
        GLuint m_ID = 0;
        GLenum m_Target = GL_TEXTURE_2D;
    };

}