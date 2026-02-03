#include "qepch.h"
#include "AssetManager.h"

#include <unordered_map>
#include <filesystem>
#include <fstream>
#include <iostream>

#include <QuasarEngine/Asset/AssetHeader.h>
#include <QuasarEngine/Resources/Model.h>
#include <QuasarEngine/Entity/Components/MeshComponent.h>
#include <QuasarEngine/Core/Logger.h>
#include <QuasarEngine/Thread/JobSystem.h>

namespace QuasarEngine
{
	AssetType AssetManager::InferTypeFromPath(const std::filesystem::path& p) const
	{
		if (p.empty())
			return AssetType::NONE;

		const std::string ext = p.extension().string();
		if (std::find(m_ValidExtention.begin(), m_ValidExtention.end(), ext) == m_ValidExtention.end())
			return AssetType::NONE;

		if (ext == ".qasset")
		{
			std::ifstream file(p, std::ios::binary);
			if (!file.is_open())
				return AssetType::NONE;

			AssetHeader assetHeader{};
			file.read(reinterpret_cast<char*>(&assetHeader), sizeof(assetHeader));
			file.close();

			return getAssetTypeFromString(assetHeader.assetType.c_str());
		}

		auto it = m_ExtentionAssetTypes.find(ext);
		if (it != m_ExtentionAssetTypes.end())
			return it->second;

		return AssetType::NONE;
	}

	AssetManager::AssetManager() {}

	AssetManager::~AssetManager()
	{
		Shutdown();
	}

	void AssetManager::Initialize(std::filesystem::path assetPath)
	{
		m_AssetPath = std::move(assetPath);

		m_AssetRegistry = std::make_unique<AssetRegistry>();

		auto& RegisterExtention = [&](const std::string& extention, AssetType type) {
			m_ValidExtention.push_back(extention);
			m_ExtentionAssetTypes[extention] = type;
			};

		RegisterExtention(".obj",		AssetType::MODEL);
		RegisterExtention(".fbx",		AssetType::MODEL);
		RegisterExtention(".dae",		AssetType::MODEL);
		RegisterExtention(".glb",		AssetType::MODEL);
		RegisterExtention(".ply",		AssetType::MODEL);
		RegisterExtention(".gltf",		AssetType::MODEL);
		RegisterExtention(".png",		AssetType::TEXTURE);
		RegisterExtention(".jpg",		AssetType::TEXTURE);
		RegisterExtention(".jpeg",		AssetType::TEXTURE);
		RegisterExtention(".qasset",		AssetType::QASSET);
		RegisterExtention(".lua",		AssetType::SCRIPT);
		RegisterExtention(".scene",		AssetType::SCENE);
		RegisterExtention(".qparticle",	AssetType::PARTICLE);
	}

	void AssetManager::Shutdown()
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
			return m_AssetRegistry->getAssetType(id);

		return AssetType::NONE;
	}

	std::filesystem::path AssetManager::ResolvePath(const std::string& path) const
	{
		if (path.empty())
			return {};

		std::filesystem::path p(path);

		return p.is_absolute() ? p : (m_AssetPath / p);
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
			case AssetType::TEXTURE:
			{
				if (isAssetLoaded(asset.id))
					break;

				//std::cout << "[AssetManager] Loading TEXTURE '" << asset.id << "'...\n";

				TextureSpecification spec;
				if (std::holds_alternative<TextureSpecification>(asset.spec))
					spec = std::get<TextureSpecification>(asset.spec);

				auto texture = Texture2D::Create(spec);

				if (asset.data)
				{
					if (spec.compressed)
					{
						texture->LoadFromMemory({ static_cast<unsigned char*>(asset.data), asset.size });
						texture->GenerateMips();
					}
					else
					{
						texture->LoadFromData({ static_cast<unsigned char*>(asset.data), asset.size });
						texture->GenerateMips();
					}
				}
				else
				{
					if (asset.path.empty())
					{
						Q_ERROR("AssetManager: TEXTURE '" + asset.id + "' error, path missing and no data");
						break;
					}
					texture->LoadFromPath(asset.path);
					texture->GenerateMips();
				}

				{
					std::lock_guard<std::mutex> lock(m_AssetMutex);
					m_LoadedAssets[asset.id] = std::move(texture);
				}

				break;
			}
			case AssetType::MODEL:
			{
				if (isAssetLoaded(asset.id))
					break;

				if (asset.path.empty()) {
					Q_ERROR("AssetManager: MODEL path missing: " + asset.id);
					break;
				}

				std::cout << "[AssetManager] Loading MODEL '" << asset.id << "'...\n";

				std::shared_ptr<Model> model;
				if (std::holds_alternative<ModelImportOptions>(asset.spec)) {
					const auto& opts = std::get<ModelImportOptions>(asset.spec);
					model = Model::CreateModel(asset.path, opts);
				}
				else {
					model = Model::CreateModel(asset.path);
				}

				{
					std::lock_guard<std::mutex> lock(m_AssetMutex);
					m_LoadedAssets[asset.id] = model;
				}
				break;
			}
			case AssetType::MESH:
			{
				if (asset.handle.has_value())
				{
					auto* mc = std::any_cast<MeshComponent*>(asset.handle);
					if (mc && isAssetLoaded(asset.id))
					{
						std::shared_ptr<Model> model = getAsset<Model>(asset.id);
						if (model)
						{
							auto meshPtr = model->FindMeshByInstanceName(mc->GetName());
							//auto meshPtr = model->FindMeshByPathAndName(mc->GetNodePath(), mc->GetName());
							if (meshPtr)
							{
								mc->m_Mesh = meshPtr.get();
							}
							else
							{
								Q_ERROR("AssetManager: MESH instance '" + mc->GetName() + "' missing on '" + asset.id + "'");
							}
						}
					}
				}
				break;
			}
			default:
				break;
			}
		}

		while (!m_AssetsToUpdate.empty())
		{
			AssetToLoad asset = m_AssetsToUpdate.front();
			m_AssetsToUpdate.pop();

			switch (asset.type)
			{
			case AssetType::TEXTURE:
			{
				TextureSpecification spec;
				if (std::holds_alternative<TextureSpecification>(asset.spec))
					spec = std::get<TextureSpecification>(asset.spec);

				auto texture = Texture2D::Create(spec);

				if (asset.data && asset.size > 0)
				{
					texture->LoadFromMemory({ static_cast<unsigned char*>(asset.data), asset.size });
					texture->GenerateMips();
				}
				else
				{
					if (asset.path.empty())
					{
						Q_ERROR("AssetManager: Update TEXTURE '" + asset.id + "' error, path missing and no data");
						continue;
					}
					texture->LoadFromPath(asset.path);
					texture->GenerateMips();
				}

				{
					std::lock_guard<std::mutex> lock(m_AssetMutex);
					m_LoadedAssets[asset.id] = std::move(texture);
				}
				break;
			}

			case AssetType::MODEL:
			{
				if (asset.path.empty())
				{
					Q_ERROR("AssetManager: Update MODEL '" + asset.id + "' error, path missing");
					continue;
				}

				std::cout << "[AssetManager] Updating MODEL '" << asset.id << "'...\n";

				std::shared_ptr<Model> model;
				if (std::holds_alternative<ModelImportOptions>(asset.spec))
				{
					const auto& opts = std::get<ModelImportOptions>(asset.spec);
					model = Model::CreateModel(asset.path, opts);
				}
				else
				{
					model = Model::CreateModel(asset.path);
				}

				{
					std::lock_guard<std::mutex> lock(m_AssetMutex);
					m_LoadedAssets[asset.id] = model;
				}
				break;
			}

			default:
				break;
			}
		}
	}

	void AssetManager::loadAsset(AssetToLoad asset)
	{
		if (!asset.data && asset.path.empty())
		{
			Q_ERROR("AssetManager: Load asset '" + asset.id + "' error, path missing and no data");
			return;
		}

		if (!m_AssetRegistry->isAssetRegistred(asset.id))
		{
			if (asset.type == AssetType::NONE)
			{
				asset.type = InferTypeFromPath(asset.path);
				m_AssetRegistry->registerAsset(asset.id, asset.type);
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
		if (!asset.data && asset.path.empty())
		{
			Q_ERROR("AssetManager: Update asset '" + asset.id + "' error, path missing and no data");
			return;
		}

		if (!m_AssetRegistry->isAssetRegistred(asset.id))
		{
			if (asset.type == AssetType::NONE)
			{
				asset.type = InferTypeFromPath(asset.path);
				m_AssetRegistry->registerAsset(asset.id, asset.type);
			}
			else
			{
				m_AssetRegistry->registerAsset(asset.id, asset.type);
			}
		}

		m_AssetsToUpdate.push(asset);
	}

	void AssetManager::unloadAsset(std::string id)
	{
		m_AssetsToUnload.push(id);
	}

	void AssetManager::instLoadAsset(std::string id, std::shared_ptr<Asset> asset)
	{
		if (!m_AssetRegistry->isAssetRegistred(id))
			m_AssetRegistry->registerAsset(id, asset->GetType());

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
		if (!asset.data && asset.path.empty())
		{
			Q_ERROR("AssetManager: Load texture async '" + asset.id + "' error, path missing and no data");
			return;
		}

		if (!m_AssetRegistry->isAssetRegistred(asset.id))
		{
			if (asset.type == AssetType::NONE)
			{
				asset.type = InferTypeFromPath(asset.path);
				m_AssetRegistry->registerAsset(asset.id, asset.type);
			}
			else
			{
				m_AssetRegistry->registerAsset(asset.id, asset.type);
			}
		}

		JobSystem::Instance().Submit(
			JobPriority::HIGH,
			JobPoolType::IO,
			[this, asset]()
			{
				if (asset.type != AssetType::TEXTURE)
					return;

				if (isAssetLoaded(asset.id))
					return;

				TextureSpecification spec;
				if (std::holds_alternative<TextureSpecification>(asset.spec))
					spec = std::get<TextureSpecification>(asset.spec);

				auto texture = Texture2D::Create(spec);

				if (asset.data)
				{
					texture->LoadFromMemory({ static_cast<unsigned char*>(asset.data), asset.size });
				}
				else
				{
					texture->LoadFromPath(asset.path);
				}

				{
					std::lock_guard<std::mutex> lock(m_AssetMutex);
					m_LoadedAssets[asset.id] = std::move(texture);
				}

				std::cout << "[JOB] Chargement texture : id=" << asset.id
					<< " path=" << asset.path << std::endl;
			}
		);
	}

	AssetType AssetManager::getTypeFromExtention(const std::string& str)
	{
		if (std::find(m_ValidExtention.begin(), m_ValidExtention.end(), str) != m_ValidExtention.end())
			return m_ExtentionAssetTypes[str];

		return AssetType::NONE;
	}

	AssetType AssetManager::getAssetTypeFromString(const char* type)
	{
		if (strcmp(type, "Texture") == 0) return AssetType::TEXTURE;
		if (strcmp(type, "Mesh") == 0)    return AssetType::MESH;
		if (strcmp(type, "Model") == 0)   return AssetType::MODEL;
		if (strcmp(type, "QAsset") == 0)  return AssetType::QASSET;
		return AssetType::NONE;
	}

	std::string AssetManager::getStringFromAssetType(AssetType type)
	{
		switch (type)
		{
		case AssetType::TEXTURE: return "Texture";
		case AssetType::MESH:    return "Mesh";
		case AssetType::MODEL:   return "Model";
		case AssetType::QASSET:  return "QAsset";
		default:                 return "None";
		}
	}

	std::shared_ptr<Asset> AssetManager::getAsset(std::string id)
	{
		if (!isAssetLoaded(id))
			return nullptr;

		std::lock_guard<std::mutex> lock(m_AssetMutex);
		return m_LoadedAssets.at(id);
	}
}