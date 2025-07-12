#include "qepch.h"
#include "Material.h"

#include <QuasarEngine/Renderer/Renderer.h>

#include "QuasarEngine/Core/Logger.h"

#define INVALID_ID 4294967295U

namespace QuasarEngine
{
	Material::Material(const MaterialSpecification& specification) :
		m_Specification(specification), m_ID(INVALID_ID), m_Generation(INVALID_ID)
	{
		if (m_Specification.AlbedoTexture.has_value())
		{
			AssetToLoad asset;
			asset.id = m_Specification.AlbedoTexture.value();
			asset.type = TEXTURE;

			TextureSpecification spec;
			spec.gamma = true;

			asset.spec = spec;

			Renderer::m_SceneData.m_AssetManager->loadAsset(asset);
		}

		if (m_Specification.NormalTexture.has_value())
		{
			AssetToLoad asset;
			asset.id = m_Specification.NormalTexture.value();
			asset.type = TEXTURE;
			Renderer::m_SceneData.m_AssetManager->loadAsset(asset);
		}

		if (m_Specification.MetallicTexture.has_value())
		{
			AssetToLoad asset;
			asset.id = m_Specification.MetallicTexture.value();
			asset.type = TEXTURE;
			Renderer::m_SceneData.m_AssetManager->loadAsset(asset);
		}

		if (m_Specification.RoughnessTexture.has_value())
		{
			AssetToLoad asset;
			asset.id = m_Specification.RoughnessTexture.value();
			asset.type = TEXTURE;
			Renderer::m_SceneData.m_AssetManager->loadAsset(asset);
		}

		if (m_Specification.AOTexture.has_value())
		{
			AssetToLoad asset;
			asset.id = m_Specification.AOTexture.value();
			asset.type = TEXTURE;
			Renderer::m_SceneData.m_AssetManager->loadAsset(asset);
		}

		m_Generation++;
	}

	Material::~Material()
	{
		
	}

	void Material::SetTexture(TextureType type, Texture2D* texture)
	{
		m_Textures[type] = texture;
	}

	void Material::SetTexture(TextureType type, std::string path)
	{
		AssetToLoad asset;

		switch (type)
		{
		case Albedo:
			m_Specification.AlbedoTexture = path;

			asset.id = path;
			asset.type = TEXTURE;
			Renderer::m_SceneData.m_AssetManager->loadAsset(asset);

			m_Generation++;
			break;
		case Normal:
			m_Specification.NormalTexture = path;

			asset.id = path;
			asset.type = TEXTURE;
			Renderer::m_SceneData.m_AssetManager->loadAsset(asset);

			m_Generation++;
			break;
		case Metallic:
			m_Specification.MetallicTexture = path;

			asset.id = path;
			asset.type = TEXTURE;
			Renderer::m_SceneData.m_AssetManager->loadAsset(asset);

			m_Generation++;
			break;
		case Roughness:
			m_Specification.RoughnessTexture = path;

			asset.id = path;
			asset.type = TEXTURE;
			Renderer::m_SceneData.m_AssetManager->loadAsset(asset);

			m_Generation++;
			break;
		case AO:
			m_Specification.AOTexture = path;

			asset.id = path;
			asset.type = TEXTURE;
			Renderer::m_SceneData.m_AssetManager->loadAsset(asset);

			m_Generation++;
			break;
		}
	}

	Texture* Material::GetTexture(TextureType type)
	{
		switch (type)
		{
		case Albedo:
			if (HasTexture(type))
				if (m_Textures.find(type) != m_Textures.end())
				{
					return m_Textures[type];
				}
				else
				{
					return Renderer::m_SceneData.m_AssetManager->getAsset<Texture>(m_Specification.AlbedoTexture.value()).get();
				}
			break;
		case Normal:
			if (HasTexture(type))
				return Renderer::m_SceneData.m_AssetManager->getAsset<Texture>(m_Specification.NormalTexture.value()).get();
			break;
		case Metallic:
			if (HasTexture(type))
				return Renderer::m_SceneData.m_AssetManager->getAsset<Texture>(m_Specification.MetallicTexture.value()).get();
			break;
		case Roughness:
			if (HasTexture(type))
				return Renderer::m_SceneData.m_AssetManager->getAsset<Texture>(m_Specification.RoughnessTexture.value()).get();
			break;
		case AO:
			if (HasTexture(type))
				return Renderer::m_SceneData.m_AssetManager->getAsset<Texture>(m_Specification.AOTexture.value()).get();
			break;
		}
		return nullptr;
	}

	bool Material::HasTexture(TextureType type)
	{
		switch (type)
		{
		case Albedo:
			return m_Specification.AlbedoTexture.has_value() || (m_Textures.find(type) != m_Textures.end());
		case Normal:
			return m_Specification.NormalTexture.has_value();
		case Metallic:
			return m_Specification.MetallicTexture.has_value();
		case Roughness:
			return m_Specification.RoughnessTexture.has_value();
		case AO:
			return m_Specification.AOTexture.has_value();
		}
		return false;
	}

	std::optional<std::string> Material::GetTexturePath(TextureType type)
	{
		switch (type)
		{
		case Albedo:
			return (HasTexture(type) ? m_Specification.AlbedoTexture : std::nullopt);
		case Normal:
			return (HasTexture(type) ? m_Specification.NormalTexture : std::nullopt);
		case Metallic:
			return (HasTexture(type) ? m_Specification.MetallicTexture : std::nullopt);
		case Roughness:
			return (HasTexture(type) ? m_Specification.RoughnessTexture : std::nullopt);
		case AO:
			return (HasTexture(type) ? m_Specification.AOTexture : std::nullopt);
		}
		return std::nullopt;
	}

	void Material::ResetTexture(TextureType type)
	{
		switch (type)
		{
		case TextureType::Albedo:
			m_Specification.AlbedoTexture.reset();
			m_Generation++;
			break;
		case TextureType::Normal:
			m_Specification.NormalTexture.reset();
			m_Generation++;
			break;
		case TextureType::Metallic:
			m_Specification.MetallicTexture.reset();
			m_Generation++;
			break;
		case TextureType::Roughness:
			m_Specification.RoughnessTexture.reset();
			m_Generation++;
			break;
		case TextureType::AO:
			m_Specification.AOTexture.reset();
			m_Generation++;
			break;
		}
	}

	std::shared_ptr<Material> Material::CreateMaterial(const MaterialSpecification& specification)
	{
		return std::make_shared<Material>(specification);
	}
}
