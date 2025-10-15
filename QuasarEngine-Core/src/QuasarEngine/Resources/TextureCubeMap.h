#pragma once

#include <memory>

#include "Texture.h"

namespace QuasarEngine
{
    class TextureCubeMap : public Texture {
    public:
        enum class Face : uint8_t { PositiveX = 0, NegativeX, PositiveY, NegativeY, PositiveZ, NegativeZ };

        explicit TextureCubeMap(const TextureSpecification& specification);
        ~TextureCubeMap() override = default;

        virtual TextureHandle GetHandle() const noexcept override = 0;
        virtual bool IsLoaded() const noexcept override = 0;

        virtual void Bind(int index = 0) const override = 0;
        virtual void Unbind() const override = 0;

        virtual bool LoadFromPath(const std::string& path) override = 0;
        virtual bool LoadFromMemory(ByteView data) override = 0;
        virtual bool LoadFromData(ByteView data) override = 0;

        virtual bool LoadFaceFromPath(Face face, const std::string& path) = 0;
        virtual bool LoadFaceFromMemory(Face face, ByteView data) = 0;
        virtual bool LoadFaceFromData(Face face, ByteView data, uint32_t w, uint32_t h, uint32_t channels) = 0;

        static std::shared_ptr<TextureCubeMap> Create(const TextureSpecification& specification);
    };
}