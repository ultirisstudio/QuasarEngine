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

        void SetTexture(const std::string& assetId, const std::string& absPath);
        const std::string& GetTextureId() const { return m_TextureId; }
        const std::string& GetTexturePath() const { return m_TexturePath; }
        void ClearTexture();
        void RebuildSystem();

        void Update(float dt);
        void Render(RenderContext& ctx);

        bool m_Enabled = true;
        bool m_Emitting = true;

        int m_MaxParticles = 512;

        float m_SpawnRate = 30.0f;
        bool m_BurstMode = false;
        int m_BurstCount = 20;
        bool m_Loop = true;

        glm::vec3 m_EmitterOffset{ 0.0f, 0.0f, 0.0f };

        glm::vec3 m_BaseDirection{ 0.0f, 1.0f, 0.0f };
        float m_SpreadAngleDegrees = 25.0f;

        float m_PositionSpread = 0.1f;
        float m_VelocitySpread = 0.0f;

        float m_SpeedMin = 0.5f;
        float m_SpeedMax = 1.5f;

        float m_LifeMin = 2.0f;
        float m_LifeMax = 4.0f;

        float m_StartSizeMin = 0.3f;
        float m_StartSizeMax = 0.6f;
        float m_EndSizeMin = 0.9f;
        float m_EndSizeMax = 1.3f;

        bool m_RandomRotation = true;
        float m_AngularVelocityMin = -2.0f;
        float m_AngularVelocityMax = 2.0f;

        glm::vec4 m_ColorStart{ 1.0f, 1.0f, 1.0f, 0.9f };
        glm::vec4 m_ColorEnd{ 0.8f, 0.8f, 0.8f, 0.0f };

        bool m_OverrideSimulation = false;
        glm::vec3 m_Gravity{ 0.0f, 1.5f, 0.0f };
        float m_LinearDrag = 1.0f;

        glm::vec3 m_Wind{ 0.0f, 0.0f, 0.0f };
        float m_TurbulenceStrength = 0.0f;
        float m_TurbulenceFrequency = 1.0f;
        float m_TurbulenceScale = 1.0f;

        float m_SizeOverLifeExponent = 1.0f;
        float m_AlphaOverLifeExponent = 1.0f;

    private:
        static float Random01();
        static float RandomRange(float min, float max);

        void EmitOne(const glm::vec3& emitterWorldPos);

    private:
        std::unique_ptr<ParticleSystem> m_System;

        std::string m_TextureId;
        std::string m_TexturePath;

        float m_SpawnTimer = 0.0f;
    };
}
