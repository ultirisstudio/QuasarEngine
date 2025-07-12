#include "qepch.h"

#include <thread>

#include <QuasarEngine/Resources/ResourceManager.h>

namespace QuasarEngine
{
	ResourceManager::ResourceManager()
	{
		//std::thread t(&ResourceManager::ResourceLoader, this);
		//t.detach();
	}

	ResourceManager::~ResourceManager()
	{

	}

	void ResourceManager::Update(double dt)
	{
		/*m_Time += dt;
		if (m_Time >= 1.0) {
			if (!m_LoadingTexturesQueue.empty())
			{
				TextureInfos infos = m_LoadingTexturesQueue.front();
				m_LoadingTexturesQueue.pop();

				m_Textures[infos.path] = std::make_shared<Texture>(infos.data, infos.size, infos.specifications);

				auto it = std::find(m_WaitingTextures.begin(), m_WaitingTextures.end(), infos.path);
				if (it != m_WaitingTextures.end()) {
					m_WaitingTextures.erase(it);
				}
			}

			m_Time = 0;
		}*/
	}

	/*void ResourceManager::ResourceLoader()
	{
		while (true)
		{
			if (m_LoadingDataQueue.empty()) continue;

			TextureInfos infos = m_LoadingDataQueue.front();
			m_LoadingDataQueue.pop();

			infos.data = Texture::LoadDataFromPath(infos.path, &infos.size);

			m_LoadingTexturesQueue.push(infos);
		}
	}*/

	void ResourceManager::Reset()
	{
		/*for (auto& texture : m_Textures)
		{
			texture.second.reset();
		}*/

		for (auto& model : m_Models)
		{
			model.second.reset();
		}

		//m_Textures.clear();
		m_Models.clear();
	}

	/*std::shared_ptr<Texture> ResourceManager::GetTexture(const std::string& id)
	{
		if (m_Textures.find(id) != m_Textures.cend())
			return m_Textures.at(id);
		else
			return nullptr;
	}*/

	std::shared_ptr<Model> ResourceManager::GetModel(const std::string& id)
	{
		if (m_Models.find(id) != m_Models.cend())
			return m_Models.at(id);
		else
			return nullptr;
	}

	/*std::shared_ptr<Texture> ResourceManager::CreateTexture(const std::string& id, const TextureSpecification& specification)
	{
		if (m_Textures.find(id) != m_Textures.cend())
			return m_Textures.at(id);
		
		m_Textures[id] = std::make_shared<Texture>(id, specification);
		return m_Textures[id];
	}

	std::shared_ptr<Texture> ResourceManager::CreateTexture(const std::string& id, std::shared_ptr<Texture> texture)
	{
		if (m_Textures.find(id) != m_Textures.cend())
			return m_Textures.at(id);

		m_Textures[id] = std::move(texture);
		return m_Textures[id];
	}*/

	std::shared_ptr<Model> ResourceManager::CreateModel(const std::string& id)
	{
		if (m_Models.find(id) != m_Models.cend())
			return m_Models.at(id);

		m_Models[id] = std::make_shared<Model>(id);
		return m_Models[id];
	}

	/*void ResourceManager::mt_CreateTexture(const std::string& id, const TextureSpecification& specification)
	{
		std::vector<std::string>::iterator it = std::find(m_WaitingTextures.begin(), m_WaitingTextures.end(), id);
		if ((m_Textures.find(id) == m_Textures.cend()) && it == m_WaitingTextures.end())
		{
			m_WaitingTextures.push_back(id);

			TextureInfos infos;
			infos.path = id;
			infos.specifications = specification;
			m_LoadingDataQueue.push(infos);
		}
	}

	std::shared_ptr<Texture> ResourceManager::UpdateTexture(const std::string& id, const TextureSpecification& specification)
	{
		if (m_Textures.find(id) != m_Textures.cend())
		{
			m_Textures[id] = std::make_shared<Texture>(id, specification);
			return m_Textures[id];
		}

		return nullptr;
	}*/
}