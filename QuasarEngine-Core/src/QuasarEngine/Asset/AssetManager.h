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

#include <QuasarEngine/Core/Singleton.h>
#include <filesystem>

namespace QuasarEngine
{
	struct AssetToLoad {
		std::string id;
		std::string path;

		AssetType type;

		size_t size = 0;
		void* data = nullptr;

		std::any handle;

		std::shared_ptr<std::vector<unsigned char>> hold;

		std::variant<std::monostate, TextureSpecification, MaterialSpecification> spec;

		AssetToLoad() : id(""), path(""), type(AssetType::NONE), size(0), data(nullptr), spec(std::monostate{})
		{
		}
	};

	inline std::filesystem::path WeakCanonical(const std::filesystem::path& p) {
		namespace fs = std::filesystem;
		try { return fs::weakly_canonical(p).lexically_normal(); }
		catch (...) { return fs::absolute(p).lexically_normal(); }
	}

	inline std::string BuildAssetIdFromAbs(const std::filesystem::path& absPath,
		const std::filesystem::path& assetsRoot) {
		namespace fs = std::filesystem;
		std::error_code ec{};
		if (!assetsRoot.empty()) {
			auto rel = fs::relative(absPath, assetsRoot, ec);
			if (!ec && !rel.empty())
				return std::string("Assets/") + rel.generic_string();
		}
		return std::string("Assets/") + absPath.filename().generic_string();
	}

	class AssetManager : public Singleton<AssetManager>
	{
	private:
		std::queue<AssetToLoad> m_AssetsToLoad;
		std::queue<AssetToLoad> m_AssetsToUpdate;
		std::queue<std::string> m_AssetsToUnload;

		std::unordered_map<std::string, std::shared_ptr<Asset>> m_LoadedAssets;

		std::unique_ptr<AssetRegistry> m_AssetRegistry;

		std::vector<std::string> m_ValidExtention;
		std::unordered_map<std::string, AssetType> m_ExtentionAssetTypes;

		std::filesystem::path m_AssetPath;

		mutable std::mutex m_AssetMutex;

		AssetType InferTypeFromPath(const std::filesystem::path& p) const;
	public:
		AssetManager();
		~AssetManager();

		void Initialize(std::filesystem::path assetPath);
		void Shutdown();

		bool isAssetRegistered(std::string id);
		void registerAsset(std::string id, AssetType type);
		AssetType getAssetType(std::string id);

		std::filesystem::path getAssetPath() const { return m_AssetPath; }
		std::filesystem::path ResolvePath(const std::string& path) const;

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