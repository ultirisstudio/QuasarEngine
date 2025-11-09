#pragma once

#include <QuasarEngine/Resources/TextureCubeMap.h>
#include <glad/glad.h>
#include <string>

namespace QuasarEngine
{
    class OpenGLTextureCubeMap : public TextureCubeMap {
    public:
        explicit OpenGLTextureCubeMap(const TextureSpecification& specification);
        ~OpenGLTextureCubeMap() override;

        TextureHandle GetHandle() const noexcept override { return static_cast<TextureHandle>(m_ID); }
        bool IsLoaded() const noexcept override { return m_Loaded; }

        bool LoadFromPath(const std::string& path) override;
        bool LoadFromMemory(ByteView data) override;
        bool LoadFromData(ByteView data) override;

        bool LoadFaceFromPath(Face face, const std::string& path) override;
        bool LoadFaceFromMemory(Face face, ByteView data) override;
        bool LoadFaceFromData(Face face, ByteView data, uint32_t w, uint32_t h, uint32_t channels) override;

        void Bind(int index = 0) const override;
        void Unbind() const override;

        bool AllocateStorage(uint32_t w, uint32_t h);
        void GenerateMips() override;

    private:
        bool UploadAllFacesDSA(ByteView allFacesPixels, bool pixelsAreFloat);
        bool UploadFaceDSA(Face face, ByteView facePixels, uint32_t w, uint32_t h, bool pixelsAreFloat);

    private:
        GLuint m_ID = 0;
        GLuint m_SamplerID = 0;
        bool m_Loaded = false;
        bool m_StorageAllocated = false;
        GLsizei m_FacesUploaded = 0;
    };
}