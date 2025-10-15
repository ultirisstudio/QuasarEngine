#pragma once

#include "Texture.h"

namespace QuasarEngine
{
	class Texture2D : public Texture
	{
	public:
		explicit Texture2D(const TextureSpecification& specification);
		virtual ~Texture2D() override = default;

		virtual TextureHandle GetHandle() const noexcept override = 0;
		virtual bool IsLoaded() const noexcept override = 0;

		virtual bool LoadFromPath(const std::string& path) override = 0;
		virtual bool LoadFromMemory(ByteView data) override = 0;
		virtual bool LoadFromData(ByteView data) override = 0;

		virtual void Bind(int index = 0) const override = 0;
		virtual void Unbind() const override = 0;

		static std::shared_ptr<Texture2D> Create(const TextureSpecification& specification);
	};
}