#pragma once

#include <memory>
#include <string>
#include <glm/glm.hpp>

#include <QuasarEngine/Entity/Component.h>
#include <QuasarEngine/Resources/Particles/ParticleSystem.h>

namespace QuasarEngine
{
    class RenderContext;

    class ParticleComponent : public Component
    {
    public:
        ParticleComponent();
        //~ParticleComponent() = default;

        const std::string& GetTextureId()   const { return m_TextureId; }
        const std::string& GetTexturePath() const { return m_TexturePath; }
        bool HasTexture() const { return !m_TextureId.empty() && !m_TexturePath.empty(); }

        void SetTexture(const std::string& assetId, const std::string& absPath);
        void ClearTexture();

        glm::vec3 m_EmitterOffset{ 0.0f, 0.0f, 0.0f };
        glm::vec3 m_BaseVelocity{ 0.0f, 0.7f, 0.0f };

        float m_PositionSpread = 0.1f;
        float m_VelocitySpread = 0.2f;

        float m_ParticleLife = 4.0f;
        float m_StartSize = 0.4f;
        float m_EndSize = 1.5f;
        float m_EndSizeJitter = 0.15f;

        glm::vec4 m_ColorStart{ 1.0f, 1.0f, 1.0f, 1.0f };
        glm::vec4 m_ColorEnd{ 1.0f, 1.0f, 1.0f, 0.0f };

        float m_SpawnInterval = 0.25f;
        float m_SpawnTimer = 0.0f;

        int m_MaxParticles = 64;

        bool m_Playing = true;
        bool m_Loop = true;
        bool m_LocalSpace = true;

        void Update(float dt, const glm::vec3& worldEmitterPos);
        void Render(RenderContext& ctx);

        void RebuildSystem();

    private:
        static float Random01();
        static float RandomRange(float min, float max);

        void EmitOne(const glm::vec3& emitterPosition);

    private:
        std::unique_ptr<ParticleSystem> m_System;

        std::string m_TextureId;
        std::string m_TexturePath;
    };
}
