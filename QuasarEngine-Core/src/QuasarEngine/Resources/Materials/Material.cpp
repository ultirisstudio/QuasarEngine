#include "qepch.h"
#include "Material.h"

#include <QuasarEngine/Renderer/Renderer.h>
#include "QuasarEngine/Core/Logger.h"

namespace QuasarEngine
{
    std::optional<std::string>& Material::idRef(TextureType type) noexcept
    {
        switch (type)
        {
        case TextureType::Albedo:   return m_Specification.AlbedoTexture;
        case TextureType::Normal:   return m_Specification.NormalTexture;
        case TextureType::Metallic: return m_Specification.MetallicTexture;
        case TextureType::Roughness:return m_Specification.RoughnessTexture;
        case TextureType::AO:       return m_Specification.AOTexture;
        default:                    return m_Specification.AlbedoTexture;
        }
    }

    const std::optional<std::string>& Material::idRef(TextureType type) const noexcept
    {
        switch (type)
        {
        case TextureType::Albedo:   return m_Specification.AlbedoTexture;
        case TextureType::Normal:   return m_Specification.NormalTexture;
        case TextureType::Metallic: return m_Specification.MetallicTexture;
        case TextureType::Roughness:return m_Specification.RoughnessTexture;
        case TextureType::AO:       return m_Specification.AOTexture;
        default:                    return m_Specification.AlbedoTexture;
        }
    }

    std::optional<TextureSpecification>& Material::specRef(TextureType type) noexcept
    {
        switch (type)
        {
        case TextureType::Albedo:   return m_Specification.AlbedoTextureSpec;
        case TextureType::Normal:   return m_Specification.NormalTextureSpec;
        case TextureType::Metallic: return m_Specification.MetallicTextureSpec;
        case TextureType::Roughness:return m_Specification.RoughnessTextureSpec;
        case TextureType::AO:       return m_Specification.AOTextureSpec;
        default:                    return m_Specification.AlbedoTextureSpec;
        }
    }

    const std::optional<TextureSpecification>& Material::specRef(TextureType type) const noexcept
    {
        switch (type)
        {
        case TextureType::Albedo:   return m_Specification.AlbedoTextureSpec;
        case TextureType::Normal:   return m_Specification.NormalTextureSpec;
        case TextureType::Metallic: return m_Specification.MetallicTextureSpec;
        case TextureType::Roughness:return m_Specification.RoughnessTextureSpec;
        case TextureType::AO:       return m_Specification.AOTextureSpec;
        default:                    return m_Specification.AlbedoTextureSpec;
        }
    }

    Material::Material(const MaterialSpecification& specification)
        : m_Specification(specification)
    {
        auto queueTex = [](const std::optional<std::string>& idOpt,
            const std::optional<TextureSpecification>& texSpecOpt,
            bool albedoGamma = false)
            {
                if (!idOpt || idOpt->empty())
                    return;

                AssetToLoad asset;
                asset.id = *idOpt;
                asset.path = AssetManager::Instance().ResolvePath(*idOpt).generic_string();
                asset.type = AssetType::TEXTURE;

                if (texSpecOpt)
                {
                    asset.spec = *texSpecOpt;
                }
                else if (albedoGamma)
                {
                    TextureSpecification ts;
                    ts.gamma = true;
                    asset.spec = ts;
                }

                AssetManager::Instance().loadAsset(asset);
            };

        queueTex(m_Specification.AlbedoTexture, m_Specification.AlbedoTextureSpec, true);
        queueTex(m_Specification.NormalTexture, m_Specification.NormalTextureSpec);
        queueTex(m_Specification.MetallicTexture, m_Specification.MetallicTextureSpec);
        queueTex(m_Specification.RoughnessTexture, m_Specification.RoughnessTextureSpec);
        queueTex(m_Specification.AOTexture, m_Specification.AOTextureSpec);
    }

    void Material::SetTexture(TextureType type, Texture2D* texture) noexcept
    {
        m_Overrides[idx(type)] = texture;
        touch();
    }

    void Material::SetTexture(TextureType type, std::string_view idProject)
    {
        idRef(type) = std::string{ idProject };

        AssetToLoad asset;
        asset.id = *idRef(type);
        asset.path = AssetManager::Instance().ResolvePath(*idRef(type)).generic_string();
        asset.type = AssetType::TEXTURE;

        if (specRef(type))
        {
            asset.spec = *specRef(type);
        }
        else if (type == TextureType::Albedo)
        {
            TextureSpecification ts;
            ts.gamma = true;
            asset.spec = ts;
        }

        AssetManager::Instance().loadAsset(asset);
        touch();
    }

    Texture* Material::GetTexture(TextureType type) const noexcept
    {
        if (auto* overridePtr = m_Overrides[idx(type)]; overridePtr)
            return overridePtr;

        const auto& idOpt = idRef(type);
        if (idOpt)
            return AssetManager::Instance().getAsset<Texture>(*idOpt).get();

        return nullptr;
    }

    bool Material::HasTexture(TextureType type) const noexcept
    {
        if (m_Overrides[idx(type)] != nullptr)
            return true;
        return idRef(type).has_value();
    }

    std::optional<std::string> Material::GetTexturePath(TextureType type) const
    {
        return idRef(type);
    }

    void Material::ResetTexture(TextureType type) noexcept
    {
        m_Overrides[idx(type)] = nullptr;
        idRef(type).reset();
        touch();
    }
}