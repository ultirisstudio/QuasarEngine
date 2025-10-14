// Material.cpp
#include "qepch.h"
#include "Material.h"
#include <QuasarEngine/Renderer/Renderer.h>
#include "QuasarEngine/Core/Logger.h"

namespace QuasarEngine {

    static TextureSpecification MakeDefaultAlbedoSpecIfMissing(const std::optional<TextureSpecification>& in) {
        if (in.has_value()) return *in;
        TextureSpecification ts{};
        ts.gamma = true; // sRGB par défaut pour l’albedo
        return ts;
    }

    Material::Material(const MaterialSpecification& specification)
        : m_Spec(specification)
    {
        auto queue = [&](TextureType t, const std::optional<std::string>& idOpt, const std::optional<TextureSpecification>& specOpt, bool albedo = false)
            {
                if (!idOpt || idOpt->empty()) return;

                AssetToLoad asset;
                asset.id = *idOpt;
                asset.path = AssetManager::Instance().ResolvePath(*idOpt).generic_string();
                asset.type = AssetType::TEXTURE;
                asset.spec = albedo ? MakeDefaultAlbedoSpecIfMissing(specOpt) : (specOpt ? *specOpt : TextureSpecification{});

                AssetManager::Instance().loadAsset(asset);
                // Optionnel: récupérer un handle si l’AssetManager le permet
                auto sp = AssetManager::Instance().getAsset<Texture>(*idOpt);
                m_Textures[Index(t)] = sp;
            };

        queue(TextureType::Albedo, m_Spec.AlbedoTexture, m_Spec.AlbedoTextureSpec, true);
        queue(TextureType::Normal, m_Spec.NormalTexture, m_Spec.NormalTextureSpec);
        queue(TextureType::Metallic, m_Spec.MetallicTexture, m_Spec.MetallicTextureSpec);
        queue(TextureType::Roughness, m_Spec.RoughnessTexture, m_Spec.RoughnessTextureSpec);
        queue(TextureType::AO, m_Spec.AOTexture, m_Spec.AOTextureSpec);
    }

    void Material::SetTexture(TextureType type, TextureHandle tex) {
        m_Textures[Index(type)] = std::move(tex);
        MarkDirty();
    }

    void Material::SetTexture(TextureType type, const std::string& assetId) {
        auto& id = idRef(type);
        auto& spec = specRef(type);

        id = assetId;

        AssetToLoad asset;
        asset.id = assetId;
        asset.path = AssetManager::Instance().ResolvePath(assetId).generic_string();
        asset.type = AssetType::TEXTURE;

        if (type == TextureType::Albedo)
            asset.spec = MakeDefaultAlbedoSpecIfMissing(spec);
        else if (spec)
            asset.spec = *spec;

        // Laisse l’AssetManager dédupliquer le chargement
        AssetManager::Instance().loadAsset(asset);
        m_Textures[Index(type)] = AssetManager::Instance().getAsset<Texture>(assetId);

        MarkDirty();
    }

    TextureHandle Material::GetTexture(TextureType type) const noexcept {
        return m_Textures[Index(type)];
    }

    bool Material::HasTexture(TextureType type) const noexcept {
        const auto& h = m_Textures[Index(type)];
        const auto& id = idRef(type);
        return (bool)h || id.has_value();
    }

    std::optional<std::string> Material::GetTexturePath(TextureType type) const noexcept {
        return idRef(type); // clair: renvoie l’id si la texture vient d’un asset
    }

    void Material::ResetTexture(TextureType type) noexcept {
        idRef(type).reset();
        specRef(type).reset();
        m_Textures[Index(type)].reset();
        MarkDirty();
    }

    std::shared_ptr<Material> Material::CreateMaterial(const MaterialSpecification& specification) {
        return std::make_shared<Material>(specification);
    }

    // ==== petits helpers pour référencer les champs de m_Spec sans switch à répétition
    static std::optional<std::string>& pickId(MaterialSpecification& s, TextureType t) {
        switch (t) {
        case TextureType::Albedo:    return s.AlbedoTexture;
        case TextureType::Normal:    return s.NormalTexture;
        case TextureType::Metallic:  return s.MetallicTexture;
        case TextureType::Roughness: return s.RoughnessTexture;
        case TextureType::AO:        return s.AOTexture;
        default: return s.AlbedoTexture; // jamais atteint
        }
    }
    static const std::optional<std::string>& pickId(const MaterialSpecification& s, TextureType t) {
        return pickId(const_cast<MaterialSpecification&>(s), t);
    }
    static std::optional<TextureSpecification>& pickSpec(MaterialSpecification& s, TextureType t) {
        switch (t) {
        case TextureType::Albedo:    return s.AlbedoTextureSpec;
        case TextureType::Normal:    return s.NormalTextureSpec;
        case TextureType::Metallic:  return s.MetallicTextureSpec;
        case TextureType::Roughness: return s.RoughnessTextureSpec;
        case TextureType::AO:        return s.AOTextureSpec;
        default: return s.AlbedoTextureSpec;
        }
    }
    static const std::optional<TextureSpecification>& pickSpec(const MaterialSpecification& s, TextureType t) {
        return pickSpec(const_cast<MaterialSpecification&>(s), t);
    }

    std::optional<std::string>& Material::idRef(TextureType t) { return pickId(m_Spec, t); }
    const std::optional<std::string>& Material::idRef(TextureType t) const { return pickId(m_Spec, t); }
    std::optional<TextureSpecification>& Material::specRef(TextureType t) { return pickSpec(m_Spec, t); }
    const std::optional<TextureSpecification>& Material::specRef(TextureType t) const { return pickSpec(m_Spec, t); }

} // namespace QuasarEngine
