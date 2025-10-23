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

		void GenerateMips() override;
		bool AllocateStorage();

	private:
		bool UploadPixelsDSA(ByteView pixels, bool pixelsAreFloat = false);

	private:
		GLuint m_ID = 0;
		bool   m_Loaded = false;
		std::string m_LastPath;
	};
}