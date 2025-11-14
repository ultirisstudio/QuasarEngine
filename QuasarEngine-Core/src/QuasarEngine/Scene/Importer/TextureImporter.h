#pragma once

#include <filesystem>
#include <fstream>

#include <QuasarEngine/File/FileUtils.h>
#include <QuasarEngine/Resources/Texture2D.h>
#include <QuasarEngine/Asset/AssetHeader.h>
#include <QuasarEngine/Asset/AssetManager.h>
#include <QuasarEngine/Scene/Importer/TextureConfigImporter.h>

namespace QuasarEngine
{
	class TextureImporter
	{
	public:
		static void exportTexture(const std::string& path, const std::string& out)
		{
			size_t size = 0;
			std::unique_ptr<unsigned char[]> data = FileUtils::ReadFileToBuffer(path, size); // (Texture2D::LoadDataFromPath(path, &size));

			TextureSpecification spec = TextureConfigImporter::ImportTextureConfig(path);

			std::ofstream file(out, std::ios::binary);

			AssetHeader assetHeader = {
				0xDEADBEEF,
				static_cast<uint32_t>(sizeof(AssetHeader) + sizeof(TextureSpecification) + size),
				"Texture"
			};
			file.write(reinterpret_cast<const char*>(&assetHeader), sizeof(assetHeader));

			file.write(reinterpret_cast<const char*>(&spec), sizeof(TextureSpecification));

			file.write(reinterpret_cast<const char*>(data.get()), size);

			file.close();
		}

		static void updateTexture(const std::string& path, const TextureSpecification& spec)
		{
			std::ifstream in_file(path, std::ios::binary);

			AssetHeader assetHeader;
			in_file.read(reinterpret_cast<char*>(&assetHeader), sizeof(assetHeader));

			TextureSpecification local_spec;
			in_file.read(reinterpret_cast<char*>(&local_spec), sizeof(TextureSpecification));

			size_t size = spec.width * spec.height * spec.channels;
			unsigned char* data = new unsigned char[size];

			in_file.read(reinterpret_cast<char*>(data), size);

			in_file.close();

			std::ofstream out_file(path, std::ios::binary);
			if (!out_file.is_open()) {
				return;
			}

			out_file.write(reinterpret_cast<const char*>(&assetHeader), sizeof(assetHeader));
			out_file.write(reinterpret_cast<const char*>(&spec), sizeof(TextureSpecification));
			out_file.write(reinterpret_cast<const char*>(data), size);

			out_file.close();
		}

		static AssetToLoad importTexture(const std::string& path) {
			unsigned char* data;

			std::ifstream file(path, std::ios::binary);

			AssetHeader assetHeader;
			file.read(reinterpret_cast<char*>(&assetHeader), sizeof(assetHeader));

			TextureSpecification spec;
			file.read(reinterpret_cast<char*>(&spec), sizeof(TextureSpecification));

			size_t size = spec.width * spec.height * spec.channels;
			data = new unsigned char[size];

			file.read(reinterpret_cast<char*>(data), size);

			file.close();

			AssetToLoad asset;
			asset.id = path;
			asset.type = TEXTURE;
			asset.size = size;
			asset.data = data;
			asset.spec = spec;

			return asset;
		}
	};
}