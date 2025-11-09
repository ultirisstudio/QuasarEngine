#pragma once

#include <QuasarEngine/Resources/TextureArray.h>
#include <glad/glad.h>
#include <vector>
#include <string>

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
        bool LoadFromFiles(const std::vector<std::string>& paths) override;

        void Bind(int index = 0) const override;
        void Unbind() const override;

        bool AllocateStorage(uint32_t width, uint32_t height, GLsizei layers);
        void GenerateMips() override;

    private:
        bool UploadPixelsDSA(ByteView pixels, GLsizei layers, bool pixelsAreFloat);

    private:
        GLuint m_ID = 0;
        GLuint m_SamplerID = 0;
        bool m_Loaded = false;
        GLsizei m_Layers = 0;
    };
}
