#pragma once

#include <QuasarEngine/Resources/TextureCubeMap.h>
#include <glad/glad.h>

namespace QuasarEngine
{
    class OpenGLTextureCubeMapView : public TextureCubeMap
    {
    public:
        OpenGLTextureCubeMapView(GLuint existingID, GLenum target, const TextureSpecification& specification)
            : TextureCubeMap(specification), m_ID(existingID), m_Target(target)
        {
            m_Specification.is_cube = true;
            m_Loaded = (m_ID != 0);
        }

        ~OpenGLTextureCubeMapView() override = default;

        TextureHandle GetHandle() const noexcept override { return static_cast<TextureHandle>(m_ID); }
        bool IsLoaded() const noexcept override { return m_Loaded; }

        bool LoadFromPath(const std::string&) override { return false; }
        bool LoadFromMemory(ByteView) override { return false; }
        bool LoadFromData(ByteView) override { return false; }

        bool LoadFaceFromPath(Face, const std::string&) override { return false; }
        bool LoadFaceFromMemory(Face, ByteView) override { return false; }
        bool LoadFaceFromData(Face, ByteView, uint32_t, uint32_t, uint32_t) override { return false; }

        void Bind(int index = 0) const override {
            if (!m_ID) return;
            glBindTextureUnit(index, m_ID);
        }
        void Unbind() const override {
        }

        void GenerateMips() override {}

    private:
        GLuint m_ID = 0;
        GLenum m_Target = GL_TEXTURE_CUBE_MAP;
        bool   m_Loaded = false;
    };
}