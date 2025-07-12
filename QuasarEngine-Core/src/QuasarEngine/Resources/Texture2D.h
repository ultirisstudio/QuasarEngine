#pragma once

#include "Texture.h"

namespace QuasarEngine
{
	class Texture2D : public Texture
	{
	public:
		explicit Texture2D(const TextureSpecification& spec);
		virtual ~Texture2D() override {};

		virtual void LoadFromPath(const std::string& path) override {};
		virtual void LoadFromMemory(unsigned char* image_data, size_t size) override {};
		virtual void LoadFromData(unsigned char* image_data, size_t size) override {};

		static unsigned char* LoadDataFromPath(const std::string& path, size_t* file_size);

		virtual void* GetHandle() const override { return nullptr; };

		virtual void Bind(int index = 0) const override {};
		virtual void Unbind() const override {};

		static std::shared_ptr<Texture2D> CreateTexture2D(const TextureSpecification& specification);
	};
}