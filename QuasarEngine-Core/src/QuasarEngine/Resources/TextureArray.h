#pragma once

#include <vector>
#include <string>

#include "Texture.h"

namespace QuasarEngine
{
	class TextureArray : public Texture
	{
	public:
		TextureArray(const TextureSpecification& specification);
		virtual ~TextureArray() override {};

		static std::shared_ptr<TextureArray> CreateTextureArray(const TextureSpecification& specification);

		virtual void Bind(int index = 0) const override {};
		virtual void Unbind() const override {};

		virtual void* GetHandle() const override { return nullptr; };

		virtual void LoadFromFiles(const std::vector<std::string>& paths) {};

		virtual void LoadFromPath(const std::string& path) override {};
		virtual void LoadFromMemory(unsigned char* image_data, size_t size) override {};
		virtual void LoadFromData(unsigned char* image_data, size_t size) override {};
	};
}