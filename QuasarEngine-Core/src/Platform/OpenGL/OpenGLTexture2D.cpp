#include "qepch.h"
#include "OpenGLTexture2D.h"

#include <stb_image.h>
#include <glad/glad.h>

#include <QuasarEngine/File/FileUtils.h>
#include <QuasarEngine/Core/Logger.h>

namespace QuasarEngine
{
	namespace Utils
	{
		static GLenum TextureTarget(bool multisampled)
		{
			return multisampled ? GL_TEXTURE_2D_MULTISAMPLE : GL_TEXTURE_2D;
		}

		static GLenum TextureFormatToGL(TextureFormat format)
		{
			switch (format)
			{
			case TextureFormat::RGB:   return GL_RGB;
			case TextureFormat::RGBA:  return GL_RGBA;
			case TextureFormat::SRGB:  return GL_SRGB;
			case TextureFormat::SRGBA: return GL_SRGB_ALPHA;
			case TextureFormat::RED:   return GL_RED;
			case TextureFormat::RED8:  return GL_R8;
			}
			return 0;
		}

		static GLenum TextureWrapToGL(TextureWrap wrap)
		{
			switch (wrap)
			{
			case TextureWrap::REPEAT:          return GL_REPEAT;
			case TextureWrap::MIRRORED_REPEAT: return GL_MIRRORED_REPEAT;
			case TextureWrap::CLAMP_TO_EDGE:   return GL_CLAMP_TO_EDGE;
			case TextureWrap::CLAMP_TO_BORDER: return GL_CLAMP_TO_BORDER;
			}
			return 0;
		}

		static GLenum TextureFilterToGL(TextureFilter filter)
		{
			switch (filter)
			{
			case TextureFilter::NEAREST:               return GL_NEAREST;
			case TextureFilter::LINEAR:                return GL_LINEAR;
			case TextureFilter::NEAREST_MIPMAP_NEAREST:return GL_NEAREST_MIPMAP_NEAREST;
			case TextureFilter::LINEAR_MIPMAP_NEAREST: return GL_LINEAR_MIPMAP_NEAREST;
			case TextureFilter::NEAREST_MIPMAP_LINEAR: return GL_NEAREST_MIPMAP_LINEAR;
			case TextureFilter::LINEAR_MIPMAP_LINEAR:  return GL_LINEAR_MIPMAP_LINEAR;
			}
			return GL_LINEAR;
		}

		static int DesiredChannelFromTextureFormat(TextureFormat format)
		{
			switch (format)
			{
			case TextureFormat::RGB:  return 3;
			case TextureFormat::RGBA: return 4;
			case TextureFormat::RED:  return 1;
			case TextureFormat::RED8: return 1;
			default: return 0;
			}
		}

		static uint32_t BytesPerPixel(GLenum format) {
			switch (format)
			{
			case GL_RGB:          return 3;
			case GL_RGBA:         return 4;
			case GL_SRGB:         return 3;
			case GL_SRGB_ALPHA:   return 4;
			case GL_RED:          return 1;
			case GL_R8:           return 1;
			default:              return 0;
			}
		}
	}

	OpenGLTexture2D::OpenGLTexture2D(const TextureSpecification& specification)
		: Texture2D(specification)
	{
	}

	OpenGLTexture2D::~OpenGLTexture2D()
	{
		glDeleteTextures(1, &m_ID);
	}

	void OpenGLTexture2D::LoadFromPath(const std::string& path)
	{
		std::vector<unsigned char> buffer;
		std::ifstream file(path, std::ios::binary);
		if (file)
		{
			file.seekg(0, std::ios::end);
			buffer.resize(file.tellg());
			file.seekg(0, std::ios::beg);
			file.read(reinterpret_cast<char*>(buffer.data()), buffer.size());
		}
		else
		{
			Q_ERROR("Failed to load texture from path: " + path);
			return;
		}

		LoadFromMemory(buffer.data(), buffer.size());
	}

	void OpenGLTexture2D::LoadFromMemory(unsigned char* image_data, size_t size)
	{
		if (m_Specification.compressed)
		{
			if (m_Specification.flip)
			{
				stbi_set_flip_vertically_on_load(true);
			}
			else
			{
				stbi_set_flip_vertically_on_load(false);
			}

			if (m_Specification.alpha)
			{
				m_Specification.format = TextureFormat::RGBA;
				m_Specification.internal_format = m_Specification.gamma ? TextureFormat::SRGBA : TextureFormat::RGBA;
			}
			else
			{
				m_Specification.format = TextureFormat::RGB;
				m_Specification.internal_format = m_Specification.gamma ? TextureFormat::SRGB : TextureFormat::RGB;
			}

			int width, height, channels;
			unsigned char* data = stbi_load_from_memory(image_data, static_cast<int>(size), &width, &height, &channels,
				Utils::DesiredChannelFromTextureFormat(m_Specification.format));

			if (!data)
			{
				Q_ERROR(std::string("Failed to decode image from memory: ") + stbi_failure_reason());
				return;
			}

			m_Specification.width = width;
			m_Specification.height = height;
			m_Specification.channels = channels;

			LoadFromData(data, width * height * Utils::DesiredChannelFromTextureFormat(m_Specification.format));

			stbi_image_free(data);
		}
		else
		{
			LoadFromData(image_data, m_Specification.width * m_Specification.height * Utils::BytesPerPixel(Utils::TextureFormatToGL(m_Specification.internal_format)));
		}
	}

	void OpenGLTexture2D::LoadFromData(unsigned char* data, size_t)
	{
		bool multisample = m_Specification.Samples > 1;

		glCreateTextures(Utils::TextureTarget(multisample), 1, &m_ID);
		glBindTexture(Utils::TextureTarget(multisample), m_ID);

		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_R, Utils::TextureWrapToGL(m_Specification.wrap_r));
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, Utils::TextureWrapToGL(m_Specification.wrap_s));
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, Utils::TextureWrapToGL(m_Specification.wrap_t));
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, Utils::TextureFilterToGL(m_Specification.min_filter_param));
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, Utils::TextureFilterToGL(m_Specification.mag_filter_param));

		glTexImage2D(
			GL_TEXTURE_2D,
			0,
			Utils::TextureFormatToGL(m_Specification.internal_format),
			m_Specification.width,
			m_Specification.height,
			0,
			Utils::TextureFormatToGL(m_Specification.format),
			GL_UNSIGNED_BYTE,
			data
		);

		if (m_Specification.mipmap)
		{
			glGenerateMipmap(GL_TEXTURE_2D);
		}
	}

	void OpenGLTexture2D::Bind(int index) const
	{
		glActiveTexture(GL_TEXTURE0 + index);
		glBindTexture(GL_TEXTURE_2D, m_ID);
	}

	void OpenGLTexture2D::Unbind() const
	{
		glBindTexture(GL_TEXTURE_2D, 0);
	}
}
