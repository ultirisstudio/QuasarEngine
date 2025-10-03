#pragma once

#include <cstdint>
#include <memory>
#include <string>
#include <vector>

#include <QuasarEngine/Asset/Asset.h>

namespace QuasarEngine
{
	enum class TextureFormat : uint8_t
	{
		RED = 0,
		RED8,
		RGB,
		RGB8,
		RGBA,
		RGBA8,
		SRGB,
		SRGB8,
		SRGBA,
		SRGB8A8,
	};

	enum class TextureWrap : uint8_t
	{
		REPEAT = 0,
		MIRRORED_REPEAT,
		CLAMP_TO_EDGE,
		CLAMP_TO_BORDER
	};

	enum class TextureFilter : uint8_t
	{
		NEAREST = 0,
		LINEAR,
		NEAREST_MIPMAP_NEAREST,
		LINEAR_MIPMAP_NEAREST,
		NEAREST_MIPMAP_LINEAR,
		LINEAR_MIPMAP_LINEAR
	};

	struct TextureSpecification
	{
		TextureFormat format = TextureFormat::RGBA;
		TextureFormat internal_format = TextureFormat::RGBA;

		TextureWrap wrap_r = TextureWrap::REPEAT;
		TextureWrap wrap_s = TextureWrap::REPEAT;
		TextureWrap wrap_t = TextureWrap::REPEAT;

		TextureFilter min_filter_param = TextureFilter::LINEAR;
		TextureFilter mag_filter_param = TextureFilter::LINEAR;

		uint32_t width = 0;
		uint32_t height = 0;

		bool alpha = true;
		bool gamma = false;
		bool flip = true;
		bool mipmap = true;
		bool compressed = true;

		uint32_t Samples = 1;
		uint32_t channels = 3;
	};

	class Texture : public Asset
	{
	protected:
		TextureSpecification m_Specification;
	public:
		explicit Texture(const TextureSpecification& specification);
		virtual ~Texture() = default;

		virtual void* GetHandle() const = 0;

		const TextureSpecification& GetSpecification() const { return m_Specification; }

		virtual void Bind(int index = 0) const = 0;
		virtual void Unbind() const = 0;

		virtual void LoadFromPath(const std::string& path) = 0;
		virtual void LoadFromMemory(unsigned char* image_data, size_t size) = 0;
		virtual void LoadFromData(unsigned char* image_data, size_t size) = 0;

		static AssetType GetStaticType() { return AssetType::TEXTURE; }
		virtual AssetType GetType() override { return GetStaticType(); }
	};

	namespace TextureUtils
	{
		static const char* TextureWrapToChar(TextureWrap wrap)
		{
			switch (wrap)
			{
			case TextureWrap::REPEAT: return "Repeat";
			case TextureWrap::MIRRORED_REPEAT: return "Mirrored Repeat";
			case TextureWrap::CLAMP_TO_EDGE: return "Clamp to Edge";
			case TextureWrap::CLAMP_TO_BORDER: return "Clamp to Border";
			}
			return "Unknown";
		}

		static const char* TextureFilterToChar(TextureFilter filter)
		{
			switch (filter)
			{
			case TextureFilter::NEAREST: return "Nearest";
			case TextureFilter::LINEAR: return "Linear";
			case TextureFilter::NEAREST_MIPMAP_NEAREST: return "Nearest Mipmap Nearest";
			case TextureFilter::LINEAR_MIPMAP_NEAREST: return "Linear Mipmap Nearest";
			case TextureFilter::NEAREST_MIPMAP_LINEAR: return "Nearest Mipmap Linear";
			case TextureFilter::LINEAR_MIPMAP_LINEAR: return "Linear Mipmap Linear";
			}
			return "Unknown";
		}

		static const char* TextureFormatToChar(TextureFormat format)
		{
			switch (format)
			{
			case TextureFormat::RED: return "RED";
			case TextureFormat::RED8: return "RED8";
			case TextureFormat::RGB: return "RGB";
			case TextureFormat::RGBA: return "RGBA";
			case TextureFormat::SRGB: return "SRGB";
			case TextureFormat::SRGBA: return "SRGBA";
			}
			return "Unknown";
		}

		static TextureWrap CharToTextureWrap(const char* wrap)
		{
			if (strcmp(wrap, "Repeat") == 0) return TextureWrap::REPEAT;
			if (strcmp(wrap, "Mirrored Repeat") == 0) return TextureWrap::MIRRORED_REPEAT;
			if (strcmp(wrap, "Clamp to Edge") == 0) return TextureWrap::CLAMP_TO_EDGE;
			if (strcmp(wrap, "Clamp to Border") == 0) return TextureWrap::CLAMP_TO_BORDER;
			return TextureWrap::REPEAT;
		}

		static TextureFilter CharToTextureFilter(const char* filter)
		{
			if (strcmp(filter, "Nearest") == 0) return TextureFilter::NEAREST;
			if (strcmp(filter, "Linear") == 0) return TextureFilter::LINEAR;
			if (strcmp(filter, "Nearest Mipmap Nearest") == 0) return TextureFilter::NEAREST_MIPMAP_NEAREST;
			if (strcmp(filter, "Linear Mipmap Nearest") == 0) return TextureFilter::LINEAR_MIPMAP_NEAREST;
			if (strcmp(filter, "Nearest Mipmap Linear") == 0) return TextureFilter::NEAREST_MIPMAP_LINEAR;
			if (strcmp(filter, "Linear Mipmap Linear") == 0) return TextureFilter::LINEAR_MIPMAP_LINEAR;
			return TextureFilter::NEAREST;
		}
	}
}