#pragma once

#include <string>
#include <unordered_map>

#include <QuasarEngine/Asset/Asset.h>

namespace QuasarEngine
{
	class AssetRegistry
	{
	private:
		std::unordered_map<std::string, AssetType> m_AssetRegistry;
	public:
		AssetType getAssetType(const std::string& id)
		{
			auto it = m_AssetRegistry.find(id);
			if (it != m_AssetRegistry.end()) {
				return it->second;
			}
			else {
				throw std::runtime_error("Asset with ID " + id + " not found");
			}
		}

		void registerAsset(const std::string& id, AssetType type)
		{
			auto it = m_AssetRegistry.find(id);
			if (it == m_AssetRegistry.end()) {
				m_AssetRegistry[id] = type;
			}
		}

		void unregisterAsset(const std::string& id)
		{
			m_AssetRegistry.erase(id);
		}

		bool isAssetRegistred(const std::string& id)
		{
			auto it = m_AssetRegistry.find(id);
			if (it != m_AssetRegistry.end()) {
				return true;
			}
			return false;
		}
	};
}