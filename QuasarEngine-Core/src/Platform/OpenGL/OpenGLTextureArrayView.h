#pragma once

#include <QuasarEngine/Resources/TextureArray.h>
#include <glad/glad.h>

namespace QuasarEngine
{
    class OpenGLTextureArrayView : public TextureArray
    {
    public:
        OpenGLTextureArrayView(GLuint existingID, GLenum target, const TextureSpecification& specification, GLsizei layers = 0)
            : TextureArray(specification), m_ID(existingID), m_Target(target), m_Layers(layers)
        {
            m_Loaded = (m_ID != 0);
        }

        ~OpenGLTextureArrayView() override = default;

        TextureHandle GetHandle() const noexcept override { return static_cast<TextureHandle>(m_ID); }
        bool IsLoaded() const noexcept override { return m_Loaded; }

        bool LoadFromPath(const std::string&) override { return false; }
        bool LoadFromMemory(ByteView) override { return false; }
        bool LoadFromData(ByteView) override { return false; }
        bool LoadFromFiles(const std::vector<std::string>&) override { return false; }

        void Bind(int index = 0) const override {
            if (!m_ID) return;
            glBindTextureUnit(index, m_ID);
        }
        void Unbind() const override {
        }

        void GenerateMips() override {}

    private:
        GLuint  m_ID = 0;
        GLenum  m_Target = GL_TEXTURE_2D_ARRAY;
        GLsizei m_Layers = 0;
        bool    m_Loaded = false;
    };
}