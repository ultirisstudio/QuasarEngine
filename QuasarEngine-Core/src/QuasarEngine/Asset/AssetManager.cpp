#include "qepch.h"
#include "AssetManager.h"

#include <unordered_map>
#include <filesystem>
#include <fstream>

#include <QuasarEngine/Asset/AssetHeader.h>
#include <QuasarEngine/Resources/Model.h>

#include <QuasarEngine/Entity/Components/MeshComponent.h>

#include <QuasarEngine/Thread/JobSystem.h>

namespace QuasarEngine
{
	AssetManager::AssetManager()
	{
		m_AssetRegistry = std::make_unique<AssetRegistry>();

		m_ValidExtention = {
			".obj",
			".fbx",
			".glb",
			".gltf",
			".png",
			".jpg",
			".jpeg",
			".qasset",
			".lua"
		};

		m_ExtentionAssetTypes[m_ValidExtention.at(0)] = AssetType::MESH;
		m_ExtentionAssetTypes[m_ValidExtention.at(1)] = AssetType::MESH;
		m_ExtentionAssetTypes[m_ValidExtention.at(2)] = AssetType::MESH;
		m_ExtentionAssetTypes[m_ValidExtention.at(3)] = AssetType::MESH;
		m_ExtentionAssetTypes[m_ValidExtention.at(4)] = AssetType::TEXTURE;
		m_ExtentionAssetTypes[m_ValidExtention.at(5)] = AssetType::TEXTURE;
		m_ExtentionAssetTypes[m_ValidExtention.at(6)] = AssetType::TEXTURE;
		m_ExtentionAssetTypes[m_ValidExtention.at(7)] = AssetType::QASSET;
		m_ExtentionAssetTypes[m_ValidExtention.at(8)] = AssetType::SCRIPT;
	}

	AssetManager::~AssetManager()
	{
		for (auto& pair : m_LoadedAssets)
		{
			if (pair.second)
				pair.second.reset();
		}
		m_LoadedAssets.clear();
	}

	bool AssetManager::isAssetRegistered(std::string id)
	{
		return m_AssetRegistry->isAssetRegistred(id);
	}

	void AssetManager::registerAsset(std::string id, AssetType type)
	{
		m_AssetRegistry->registerAsset(id, type);
	}

	AssetType AssetManager::getAssetType(std::string id)
	{
		if (m_AssetRegistry->isAssetRegistred(id))
		{
			return m_AssetRegistry->getAssetType(id);
		}
		else
		{
			std::string extention = std::filesystem::path(id).extension().string();
			if (std::find(m_ValidExtention.begin(), m_ValidExtention.end(), extention) != m_ValidExtention.end())
			{
				if (strcmp(extention.c_str(), ".qasset") == 0)
				{
					std::ifstream file(id, std::ios::binary);

					AssetHeader assetHeader;
					file.read(reinterpret_cast<char*>(&assetHeader), sizeof(assetHeader));

					std::string type = assetHeader.assetType;

					file.close();


					return getAssetTypeFromString(type.c_str());
				}
				else
				{
					return m_ExtentionAssetTypes.at(extention);
				}
			}
		}

		return AssetType::NONE;
	}

	void AssetManager::Update()
	{
		while (!m_AssetsToUnload.empty())
		{
			std::string id = m_AssetsToUnload.front();
			m_AssetsToUnload.pop();

			std::lock_guard<std::mutex> lock(m_AssetMutex);
			auto it = m_LoadedAssets.find(id);
			if (it != m_LoadedAssets.end())
			{
				m_LoadedAssets[id].reset();
				m_LoadedAssets.erase(it);
			}
		}

		while (!m_AssetsToLoad.empty())
		{
			AssetToLoad asset = m_AssetsToLoad.front();
			m_AssetsToLoad.pop();

			switch (asset.type)
			{
			case TEXTURE:
			{
				if (isAssetLoaded(asset.id))
				{
					return;
				}

				TextureSpecification spec;
				if (std::holds_alternative<TextureSpecification>(asset.spec))
					spec = std::get<TextureSpecification>(asset.spec);

				std::shared_ptr<Texture2D> texture = Texture2D::CreateTexture2D(spec);
				if (asset.data)
				{
					texture->LoadFromMemory(static_cast<unsigned char*>(asset.data), asset.size);
				}
				else if (!asset.id.empty())
				{
					texture->LoadFromPath(asset.id);
				}

				std::lock_guard<std::mutex> lock(m_AssetMutex);
				m_LoadedAssets[asset.id] = std::move(texture);
				break;
			}
			case MODEL:
			{
				if (isAssetLoaded(asset.id))
				{
					return;
				}

				std::shared_ptr<Model> model = Model::CreateModel(asset.id);
				std::lock_guard<std::mutex> lock(m_AssetMutex);
				m_LoadedAssets[asset.id] = model;
				break;
			}
			case MESH:
				if (asset.handle.has_value())
				{
					auto* mc = std::any_cast<MeshComponent*>(asset.handle);

					if (mc)
					{
						if (isAssetLoaded(asset.id))
						{
							std::shared_ptr<Model> model = getAsset<Model>(asset.id);
							mc->m_Mesh = model->GetMesh(mc->GetName());
						}
					}
					else
					{
						
					}
				}
				else
				{
					
				}
				break;
			default:
				break;
			}
		}

		while (!m_AssetsToUpdate.empty())
		{
			AssetToLoad asset = m_AssetsToUpdate.front();
			m_AssetsToUpdate.pop();

			std::lock_guard<std::mutex> lock(m_AssetMutex);
			auto it = m_LoadedAssets.find(asset.id);
			if (it == m_LoadedAssets.end())
				continue;

			it->second = Texture2D::CreateTexture2D(std::get<TextureSpecification>(asset.spec));
			std::shared_ptr<Asset> base = it->second;
			auto texture = std::dynamic_pointer_cast<Texture2D>(base);

			if (asset.data && asset.size > 0)
			{
				texture->LoadFromMemory(static_cast<unsigned char*>(asset.data), asset.size);
			}
			else if (!asset.id.empty())
			{
				texture->LoadFromPath(asset.id);
			}
		}
	}

	void AssetManager::loadAsset(AssetToLoad asset)
	{
		if (!m_AssetRegistry->isAssetRegistred(asset.id))
		{
			if (asset.type == NONE)
			{
				AssetType type = getAssetType(asset.id);
				asset.type = type;
				m_AssetRegistry->registerAsset(asset.id, type);
			}
			else
			{
				m_AssetRegistry->registerAsset(asset.id, asset.type);
			}
		}

		m_AssetsToLoad.push(asset);
	}

	void AssetManager::updateAsset(AssetToLoad asset)
	{
		m_AssetsToUpdate.push(asset);
	}

	void AssetManager::unloadAsset(std::string id)
	{
		m_AssetsToUnload.push(id);
	}

	void AssetManager::instLoadAsset(std::string id, std::shared_ptr<Asset> asset)
	{
		if (!m_AssetRegistry->isAssetRegistred(id))
		{
			m_AssetRegistry->registerAsset(id, asset->GetType());
		}

		std::lock_guard<std::mutex> lock(m_AssetMutex);
		m_LoadedAssets[id] = std::move(asset);
	}

	bool AssetManager::isAssetLoaded(std::string id) const
	{
		std::lock_guard<std::mutex> lock(m_AssetMutex);
		return (m_LoadedAssets.find(id) != m_LoadedAssets.end());
	}

	void AssetManager::LoadTextureAsync(AssetToLoad asset)
	{
		if (!m_AssetRegistry->isAssetRegistred(asset.id))
		{
			if (asset.type == NONE)
			{
				AssetType type = getAssetType(asset.id);
				asset.type = type;
				m_AssetRegistry->registerAsset(asset.id, type);
			}
			else
			{
				m_AssetRegistry->registerAsset(asset.id, asset.type);
			}
		}

		JobSystem::instance().Submit([this, asset]() {
			if (asset.type != TEXTURE)
			{
				return;
			}

			{
				if (isAssetLoaded(asset.id))
				{
					return;
				}
			}

			TextureSpecification spec;
			if (std::holds_alternative<TextureSpecification>(asset.spec))
				spec = std::get<TextureSpecification>(asset.spec);

			std::shared_ptr<Texture2D> texture = Texture2D::CreateTexture2D(spec);
			if (asset.data)
			{
				texture->LoadFromMemory(static_cast<unsigned char*>(asset.data), asset.size);
			}
			else if (!asset.id.empty())
			{
				texture->LoadFromPath(asset.id);
			}

			{
				std::lock_guard<std::mutex> lock(m_AssetMutex);
				m_LoadedAssets[asset.id] = std::move(texture);
			}

			std::cout << "[JOB] Chargement texture lancé : " << asset.id << std::endl;

			}, JobPriority::HIGH, JobPoolType::IO, {}, "LoadTextureAsync");
	}

	AssetType AssetManager::getTypeFromExtention(const std::string& str)
	{
		if (std::find(m_ValidExtention.begin(), m_ValidExtention.end(), str) != m_ValidExtention.end())
		{
			return m_ExtentionAssetTypes[str];
		}

		return AssetType::NONE;
	}

	AssetType AssetManager::getAssetTypeFromString(const char* type)
	{
		if (strcmp(type, "Texture") == 0) return AssetType::TEXTURE;
		if (strcmp(type, "Mesh") == 0) return AssetType::MESH;
		if (strcmp(type, "Model") == 0) return AssetType::MODEL;
		if (strcmp(type, "QAsset") == 0) return AssetType::QASSET;
		return AssetType::NONE;
	}

	std::string AssetManager::getStringFromAssetType(AssetType type)
	{
		switch (type)
		{
		case AssetType::TEXTURE: return "Texture";
		case AssetType::MESH: return "Mesh";
		case AssetType::MODEL: return "Model";
		case AssetType::QASSET: return "QAsset";
		}

		return "None";
	}

	std::shared_ptr<Asset> AssetManager::getAsset(std::string id)
	{
		if (!isAssetLoaded(id))
			return nullptr;

		std::lock_guard<std::mutex> lock(m_AssetMutex);
		return m_LoadedAssets.at(id);
	}
}
