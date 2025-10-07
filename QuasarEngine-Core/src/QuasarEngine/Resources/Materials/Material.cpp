#include "qepch.h"
#include "Material.h"

#include <QuasarEngine/Renderer/Renderer.h>
#include "QuasarEngine/Core/Logger.h"

#define INVALID_ID 4294967295U

namespace QuasarEngine
{
	Material::Material(const MaterialSpecification& specification)
		: m_Specification(specification), m_ID(INVALID_ID), m_Generation(INVALID_ID)
	{
		auto queueTex = [](const std::optional<std::string>& idOpt, bool albedoGamma = false)
			{
				if (!idOpt.has_value() || idOpt->empty()) return;

				AssetToLoad asset;
				asset.id = *idOpt;
				asset.path = AssetManager::Instance().ResolvePath(*idOpt).generic_string();
				asset.type = AssetType::TEXTURE;

				if (albedoGamma)
				{
					TextureSpecification ts;
					ts.gamma = true;
					asset.spec = ts;
				}

				AssetManager::Instance().loadAsset(asset);
			};

		queueTex(m_Specification.AlbedoTexture, true);
		queueTex(m_Specification.NormalTexture);
		queueTex(m_Specification.MetallicTexture);
		queueTex(m_Specification.RoughnessTexture);
		queueTex(m_Specification.AOTexture);

		m_Generation++;
	}

	Material::~Material() {}

	void Material::SetTexture(TextureType type, Texture2D* texture)
	{
		m_Textures[type] = texture;
	}

	void Material::SetTexture(TextureType type, std::string idProject)
	{
		AssetToLoad asset;
		asset.id = idProject;
		asset.path = AssetManager::Instance().ResolvePath(idProject).generic_string();
		asset.type = AssetType::TEXTURE;

		switch (type)
		{
		case Albedo:
			m_Specification.AlbedoTexture = idProject;
			{
				TextureSpecification ts;
				ts.gamma = true;
				asset.spec = ts;
			}
			AssetManager::Instance().loadAsset(asset);
			m_Generation++;
			break;

		case Normal:
			m_Specification.NormalTexture = idProject;
			AssetManager::Instance().loadAsset(asset);
			m_Generation++;
			break;

		case Metallic:
			m_Specification.MetallicTexture = idProject;
			AssetManager::Instance().loadAsset(asset);
			m_Generation++;
			break;

		case Roughness:
			m_Specification.RoughnessTexture = idProject;
			AssetManager::Instance().loadAsset(asset);
			m_Generation++;
			break;

		case AO:
			m_Specification.AOTexture = idProject;
			AssetManager::Instance().loadAsset(asset);
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
			{
				if (m_Textures.find(type) != m_Textures.end())
					return m_Textures[type];
				return AssetManager::Instance().getAsset<Texture>(m_Specification.AlbedoTexture.value()).get();
			}
			break;

		case Normal:
			if (HasTexture(type))
				return AssetManager::Instance().getAsset<Texture>(m_Specification.NormalTexture.value()).get();
			break;

		case Metallic:
			if (HasTexture(type))
				return AssetManager::Instance().getAsset<Texture>(m_Specification.MetallicTexture.value()).get();
			break;

		case Roughness:
			if (HasTexture(type))
				return AssetManager::Instance().getAsset<Texture>(m_Specification.RoughnessTexture.value()).get();
			break;

		case AO:
			if (HasTexture(type))
				return AssetManager::Instance().getAsset<Texture>(m_Specification.AOTexture.value()).get();
			break;
		}
		return nullptr;
	}

	bool Material::HasTexture(TextureType type)
	{
		switch (type)
		{
		case Albedo:   return m_Specification.AlbedoTexture.has_value() || (m_Textures.find(type) != m_Textures.end());
		case Normal:   return m_Specification.NormalTexture.has_value();
		case Metallic: return m_Specification.MetallicTexture.has_value();
		case Roughness:return m_Specification.RoughnessTexture.has_value();
		case AO:       return m_Specification.AOTexture.has_value();
		}
		return false;
	}

	std::optional<std::string> Material::GetTexturePath(TextureType type)
	{
		switch (type)
		{
		case Albedo:    return (HasTexture(type) ? m_Specification.AlbedoTexture : std::nullopt);
		case Normal:    return (HasTexture(type) ? m_Specification.NormalTexture : std::nullopt);
		case Metallic:  return (HasTexture(type) ? m_Specification.MetallicTexture : std::nullopt);
		case Roughness: return (HasTexture(type) ? m_Specification.RoughnessTexture : std::nullopt);
		case AO:        return (HasTexture(type) ? m_Specification.AOTexture : std::nullopt);
		}
		return std::nullopt;
	}

	void Material::ResetTexture(TextureType type)
	{
		switch (type)
		{
		case TextureType::Albedo:    m_Specification.AlbedoTexture.reset();    m_Generation++; break;
		case TextureType::Normal:    m_Specification.NormalTexture.reset();    m_Generation++; break;
		case TextureType::Metallic:  m_Specification.MetallicTexture.reset();  m_Generation++; break;
		case TextureType::Roughness: m_Specification.RoughnessTexture.reset(); m_Generation++; break;
		case TextureType::AO:        m_Specification.AOTexture.reset();        m_Generation++; break;
		}
	}

	std::shared_ptr<Material> Material::CreateMaterial(const MaterialSpecification& specification)
	{
		return std::make_shared<Material>(specification);
	}
}