#pragma once

#include <memory>
#include <string>
#include <queue>
#include <variant>
#include <any>
#include <future>
#include <mutex>

#include "Asset.h"
#include "AssetRegistry.h"

#include <QuasarEngine/Resources/Texture2D.h>
#include <QuasarEngine/Resources/Materials/Material.h>

namespace QuasarEngine
{
	struct AssetToLoad {
		std::string id;
		AssetType type;

		size_t size = 0;
		void* data = nullptr;

		std::any handle;

		std::variant<std::monostate, TextureSpecification, MaterialSpecification> spec;
	};

	class AssetManager
	{
	private:
		std::queue<AssetToLoad> m_AssetsToLoad;
		std::queue<AssetToLoad> m_AssetsToUpdate;
		std::queue<std::string> m_AssetsToUnload;

		std::unordered_map<std::string, std::shared_ptr<Asset>> m_LoadedAssets;

		std::unique_ptr<AssetRegistry> m_AssetRegistry;

		std::vector<std::string> m_ValidExtention;
		std::unordered_map<std::string, AssetType> m_ExtentionAssetTypes;

		mutable std::mutex m_AssetMutex;
	public:
		AssetManager();
		~AssetManager();

		bool isAssetRegistered(std::string id);
		void registerAsset(std::string id, AssetType type);

		AssetType getAssetType(std::string id);

		void Update();

		void loadAsset(AssetToLoad asset);
		void updateAsset(AssetToLoad asset);
		void unloadAsset(std::string id);

		void instLoadAsset(std::string id, std::shared_ptr<Asset> asset);

		std::shared_ptr<Asset> getAsset(std::string id);

		bool isAssetLoaded(std::string id) const;

		template<typename T>
		std::shared_ptr<T> getAsset(std::string id)
		{
			return std::dynamic_pointer_cast<T>(getAsset(id));
		}

		void LoadTextureAsync(AssetToLoad asset);

		AssetType getTypeFromExtention(const std::string& str);

		static AssetType getAssetTypeFromString(const char* type);

		static std::string getStringFromAssetType(AssetType type);
	};
}