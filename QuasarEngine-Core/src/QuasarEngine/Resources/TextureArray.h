#pragma once

#include <vector>
#include <string>

#include "Texture.h"

namespace QuasarEngine
{
    class TextureArray : public Texture {
    public:
        explicit TextureArray(const TextureSpecification& specification);
        ~TextureArray() override = default;

        virtual TextureHandle GetHandle() const noexcept override = 0;
        virtual bool IsLoaded() const noexcept override = 0;

        virtual void Bind(int index = 0) const override = 0;
        virtual void Unbind() const override = 0;

        virtual bool LoadFromPath(const std::string& path) override = 0;
        virtual bool LoadFromMemory(ByteView data) override = 0;
        virtual bool LoadFromData(ByteView data) override = 0;

        virtual bool LoadFromFiles(const std::vector<std::string>& paths) = 0;

        static std::shared_ptr<TextureArray> Create(const TextureSpecification& specification);
    };
}