#pragma once

#include <map>
#include <string>

#include <QuasarEngine/Asset/Asset.h>
#include <QuasarEngine/Resources/Model.h>
#include <QuasarEngine/Resources/Texture.h>
#include <QuasarEngine/Thread/TQueue.h>

namespace QuasarEngine
{
	class ResourceManager
	{
	private:
		//std::map<std::string, std::shared_ptr<Texture>> m_Textures;
		std::map<std::string, std::shared_ptr<Model>> m_Models;

	private:
		/*struct TextureInfos
		{
			std::string path;
			TextureSpecification specifications;
			unsigned char* data;
			size_t size;
		};

		TSQueue<TextureInfos> m_LoadingTexturesQueue;
		TSQueue<TextureInfos> m_LoadingDataQueue;

		std::vector<std::string> m_WaitingTextures;

		double m_Time = 0;*/

	public:
		ResourceManager();
		~ResourceManager();

		void Update(double dt);
		//void ResourceLoader();
		void Reset();

		//std::shared_ptr<Texture> GetTexture(const std::string& id);
		std::shared_ptr<Model> GetModel(const std::string& id);

		//std::shared_ptr<Texture> CreateTexture(const std::string& id, const TextureSpecification& specification);
		//std::shared_ptr<Texture> CreateTexture(const std::string& id, std::shared_ptr<Texture> texture);
		std::shared_ptr<Model> CreateModel(const std::string& id);

		//void mt_CreateTexture(const std::string& id, const TextureSpecification& specification);

		//std::shared_ptr<Texture> UpdateTexture(const std::string& id, const TextureSpecification& specification);
	};
}