#pragma once

#include <filesystem>
#include <QuasarEngine/Resources/Texture.h>

namespace QuasarEngine
{
	class TextureConfigImporter
	{
	public:
		static TextureSpecification ImportTextureConfig(std::filesystem::path path);
		static void ExportTextureConfig(std::filesystem::path path, const TextureSpecification& specification);
	};
}