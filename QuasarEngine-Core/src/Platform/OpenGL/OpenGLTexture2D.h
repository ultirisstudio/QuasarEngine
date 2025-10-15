#pragma once

#include <QuasarEngine/Resources/Texture2D.h>

#include <glad/glad.h>

namespace QuasarEngine
{
	class OpenGLTexture2D : public Texture2D
	{
	public:
		OpenGLTexture2D(const TextureSpecification& specification);
		~OpenGLTexture2D() override;

		TextureHandle GetHandle() const noexcept override { return static_cast<TextureHandle>(m_ID); }
		bool IsLoaded() const noexcept override { return m_Loaded; }

		bool LoadFromPath(const std::string& path) override;
		bool LoadFromMemory(ByteView data) override;
		bool LoadFromData(ByteView data) override;

		void Bind(int index = 0) const override;
		void Unbind() const override;

		GLuint m_ID = 0;

	private:
		bool UploadPixelsDSA(ByteView pixels);

	private:
		bool   m_Loaded = false;
	};
}