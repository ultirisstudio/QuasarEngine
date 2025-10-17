#include "qepch.h"
#include "SpriteComponent.h"

#include <QuasarEngine/Asset/AssetManager.h>
#include <QuasarEngine/Core/Logger.h>

namespace QuasarEngine
{
    SpriteComponent::SpriteComponent(const SpriteSpecification& spec)
        : m_Spec(spec)
    {
        if (m_Spec.TextureId && !m_Spec.TextureId->empty())
        {
            AssetToLoad a;
            a.id = *m_Spec.TextureId;
            a.path = AssetManager::Instance().ResolvePath(*m_Spec.TextureId).generic_string();
            a.type = AssetType::TEXTURE;

            if (m_Spec.TextureSpec) a.spec = *m_Spec.TextureSpec;

            AssetManager::Instance().loadAsset(a);
        }
    }

    void SpriteComponent::SetTexture(Texture2D* texture) noexcept
    {
        m_OverrideTexture = texture;
        touch();
    }

    void SpriteComponent::SetTexture(std::string_view idProject)
    {
        m_Spec.TextureId = std::string{ idProject };

        AssetToLoad a;
        a.id = *m_Spec.TextureId;
        a.path = AssetManager::Instance().ResolvePath(*m_Spec.TextureId).generic_string();
        a.type = AssetType::TEXTURE;
        if (m_Spec.TextureSpec) a.spec = *m_Spec.TextureSpec;

        AssetManager::Instance().loadAsset(a);
        touch();
    }

    void SpriteComponent::ResetTexture() noexcept
    {
        m_OverrideTexture = nullptr;
        m_Spec.TextureId.reset();
        touch();
    }

    Texture* SpriteComponent::GetTexture() const noexcept
    {
        if (m_OverrideTexture) return m_OverrideTexture;
        if (m_Spec.TextureId)
            return AssetManager::Instance().getAsset<Texture>(*m_Spec.TextureId).get();
        return nullptr;
    }

    bool SpriteComponent::HasTexture() const noexcept
    {
        if (m_OverrideTexture) return true;
        return m_Spec.TextureId.has_value();
    }

    glm::vec4 SpriteComponent::GetEffectiveUV() const noexcept
    {
        // uv = (umin, vmin, umax, vmax) avec flips éventuels
        glm::vec4 uv = m_Spec.UV;

        if (m_Spec.FlipX) std::swap(uv.x, uv.z);
        if (m_Spec.FlipY) std::swap(uv.y, uv.w);

        // Tiling+Offset : à appliquer côté shader en général,
        // mais si tu veux le “cuire” ici, tu peux faire :
        // uv.xy = uv.xy * m_Spec.Tiling + m_Spec.Offset;
        // uv.zw = uv.zw * m_Spec.Tiling + m_Spec.Offset;

        return uv;
    }
}