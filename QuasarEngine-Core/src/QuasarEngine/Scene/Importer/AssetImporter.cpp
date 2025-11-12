#include "qepch.h"
#include "AssetImporter.h"

#include "TextureImporter.h"

#include <QuasarEngine/Core/Logger.h>
#include <QuasarEngine/Renderer/Renderer.h>
#include <QuasarEngine/Asset/Asset.h>
#include <QuasarEngine/Asset/AssetHeader.h>
#include <QuasarEngine/Tools/Utils.h>

namespace QuasarEngine
{
	namespace ImporterUtils
	{
		static AssetType getAssetTypeFromString(const char* type)
		{
			if (strcmp(type, "Texture") == 0) return AssetType::TEXTURE;
			return AssetType::NONE;
		}
	}

	AssetImporter::AssetImporter(const std::string& projectPath) : m_ProjectPath(projectPath)
	{
		m_ValidExtention = {
			//".obj",
			//".fbx",
			//".glb",
			//".gltf",
			".png",
			".jpg",
			".jpeg",
			".qasset"
		};

		m_ImportFunctions[m_ValidExtention.at(0)] = &TextureImporter::importTexture;
		m_ExportFunctions[m_ValidExtention.at(0)] = &TextureImporter::exportTexture;

		m_ImportFunctions[m_ValidExtention.at(1)] = &TextureImporter::importTexture;
		m_ExportFunctions[m_ValidExtention.at(1)] = &TextureImporter::exportTexture;

		m_ImportFunctions[m_ValidExtention.at(2)] = &TextureImporter::importTexture;
		m_ExportFunctions[m_ValidExtention.at(2)] = &TextureImporter::exportTexture;
	}

	void AssetImporter::ImportAsset()
	{
		std::optional<std::vector<Utils::FileInfo>> infos = Utils::openFiles();

		if (infos.has_value())
		{
			for (const auto& info : infos.value()) {
				Q_INFO(info.selectedFile);
			}

			char* path = Utils::openFolder();

			Q_INFO(path);

			for (const auto& info : infos.value())
			{
				if (std::find(m_ValidExtention.begin(), m_ValidExtention.end(), info.fileExtension) != m_ValidExtention.end())
				{
					ExportFunction::iterator it = m_ExportFunctions.find(info.fileExtension);
					if (it != m_ExportFunctions.end())
					{
						std::string out = std::string(std::string(path) + "\\" + info.fileName + ".qasset");
						it->second(info.filePath, out);
						Q_INFO(std::string(info.selectedFile + " imported on " + out));
					}
				}
			}
		}
	}

	void AssetImporter::ImportAsset(std::filesystem::path path)
	{
		if (std::find(m_ValidExtention.begin(), m_ValidExtention.end(), path.extension().string()) != m_ValidExtention.end())
		{
			/*Log::log(Log::INFO, "extention valid");
			std::unordered_map<std::string, std::function<std::vector<char>(std::filesystem::path)>>::iterator it = m_AssetData.m_ImportFunction.find(path.extension().string());
			if (it != m_AssetData.m_ImportFunction.end())
			{
				Log::log(Log::INFO, "call function");
				std::vector<char> data = it->second(path);
			}*/
			if (strcmp(path.extension().string().c_str(), ".qasset") == 0)
			{
				
			}
		}
	}

	AssetType AssetImporter::getAssetType(std::filesystem::path path)
	{
		std::ifstream file(path, std::ios::binary);

		AssetHeader assetHeader;
		file.read(reinterpret_cast<char*>(&assetHeader), sizeof(assetHeader));

		file.close();

		std::string type = assetHeader.assetType;

		return ImporterUtils::getAssetTypeFromString(type.c_str());
	}
}