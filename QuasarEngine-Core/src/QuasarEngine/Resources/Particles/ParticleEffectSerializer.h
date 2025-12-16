#pragma once

#include <string>
#include <filesystem>

namespace YAML { class Emitter; class Node; }

namespace QuasarEngine
{
    class ParticleEffect;

    class ParticleEffectSerializer
    {
    public:
        ParticleEffectSerializer() = default;
        explicit ParticleEffectSerializer(ParticleEffect* effect) : m_Effect(effect) {}

        void SetEffect(ParticleEffect* effect) { m_Effect = effect; }

        bool Serialize(const std::string& filepath) const;
        bool Deserialize(const std::string& filepath);

    private:
        ParticleEffect* m_Effect = nullptr;
    };
}
