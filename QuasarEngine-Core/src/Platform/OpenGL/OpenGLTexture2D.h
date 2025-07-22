#pragma once

#include <QuasarEngine/Resources/Texture2D.h>

namespace QuasarEngine
{
	class OpenGLTexture2D : public Texture2D
	{
	public:
		OpenGLTexture2D(const TextureSpecification& specification);
		~OpenGLTexture2D() override;

		void LoadFromPath(const std::string& path) override;
		void LoadFromMemory(unsigned char* image_data, size_t size) override;
		void LoadFromData(unsigned char* image_data, size_t size) override;

		void* GetHandle() const override { return reinterpret_cast<void*>(static_cast<uintptr_t>(m_ID)); }

		void Bind(int index) const override;
		void Unbind() const override;

		uint32_t m_ID;
	};
}