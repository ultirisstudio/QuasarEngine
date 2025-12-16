#pragma once

#include <string>
#include <vector>
#include <glm/glm.hpp>

#include <QuasarEngine/Asset/Asset.h>

namespace QuasarEngine
{
    enum class ParticleEmitterShape
    {
        Point = 0,
        Sphere,
        Box,
        Cone
    };

    enum class ParticleEmitterSimulationSpace
    {
        World = 0,
        Local
    };

    enum class ParticleEmitterBlendMode
    {
        Alpha = 0,
        Additive,
        Multiply
    };

    struct ParticleBurst
    {
        float time = 0.0f;
        int count = 0;
    };

    struct ParticleEmitterSettings
    {
        std::string name = "Emitter";

        bool enabled = true;
        bool loop = true;
        float duration = 0.0f;

        ParticleEmitterSimulationSpace simulationSpace = ParticleEmitterSimulationSpace::World;

        float spawnRate = 10.0f;
        int maxParticles = 512;
        std::vector<ParticleBurst> bursts;

        ParticleEmitterShape shape = ParticleEmitterShape::Point;
        glm::vec3 positionOffset{ 0.0f, 0.0f, 0.0f };

        float sphereRadius = 0.0f;
        glm::vec3 boxExtents{ 0.0f, 0.0f, 0.0f };
        float coneAngle = 0.0f;
        float coneRadius = 0.0f;
        float coneLength = 0.0f;

        glm::vec3 baseDirection{ 0.0f, 1.0f, 0.0f };
        float spreadAngleDeg = 0.0f;
        float speedMin = 1.0f;
        float speedMax = 1.0f;
        float velocityRandomness = 0.0f;

        float lifeMin = 1.0f;
        float lifeMax = 1.0f;

        float startSizeMin = 1.0f;
        float startSizeMax = 1.0f;
        float endSizeMin = 1.0f;
        float endSizeMax = 1.0f;

        float sizeOverLifeExponent = 1.0f;
        float alphaOverLifeExponent = 1.0f;

        bool randomRotation = true;
        float angularVelocityMin = 0.0f;
        float angularVelocityMax = 0.0f;

        glm::vec4 colorStart{ 1.0f, 1.0f, 1.0f, 1.0f };
        glm::vec4 colorEnd{ 1.0f, 1.0f, 1.0f, 0.0f };

        glm::vec3 gravity{ 0.0f, 0.0f, 0.0f };
        float linearDrag = 0.0f;

        glm::vec3 wind{ 0.0f, 0.0f, 0.0f };
        float turbulenceStrength = 0.0f;
        float turbulenceFrequency = 1.0f;
        float turbulenceScale = 1.0f;

        ParticleEmitterBlendMode blendMode = ParticleEmitterBlendMode::Alpha;
        bool softFade = true;
    };

    struct ParticleEmitterAsset
    {
        ParticleEmitterSettings settings;
        std::string textureId;
        std::string texturePath;
    };

    class ParticleEffect : public Asset
    {
    public:
        ParticleEffect() = default;
        explicit ParticleEffect(std::string name) : m_Name(std::move(name)) {}

        const std::string& GetName() const noexcept { return m_Name; }
        void SetName(const std::string& n) { m_Name = n; }

        std::vector<ParticleEmitterAsset>& GetEmitters() { return m_Emitters; }
        const std::vector<ParticleEmitterAsset>& GetEmitters() const { return m_Emitters; }

        std::size_t GetEmitterCount() const { return m_Emitters.size(); }

        ParticleEmitterAsset* GetEmitter(std::size_t index)
        {
            if (index >= m_Emitters.size()) return nullptr;
            return &m_Emitters[index];
        }

        const ParticleEmitterAsset* GetEmitter(std::size_t index) const
        {
            if (index >= m_Emitters.size()) return nullptr;
            return &m_Emitters[index];
        }

        ParticleEmitterAsset& AddEmitter(const std::string& name = "Emitter")
        {
            ParticleEmitterAsset e;
            e.settings.name = name;
            m_Emitters.push_back(e);
            return m_Emitters.back();
        }

        void RemoveEmitter(std::size_t index)
        {
            if (index >= m_Emitters.size()) return;
            m_Emitters.erase(m_Emitters.begin() + index);
        }

        void ClearEmitters()
        {
            m_Emitters.clear();
        }

        static AssetType GetStaticType() { return AssetType::PARTICLE; }
        AssetType GetType() override { return GetStaticType(); }

    private:
        std::string m_Name = "ParticleEffect";
        std::vector<ParticleEmitterAsset> m_Emitters;
    };
}
