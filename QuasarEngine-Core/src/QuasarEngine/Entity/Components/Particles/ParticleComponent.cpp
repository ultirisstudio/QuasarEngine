#include "qepch.h"
#include "ParticleComponent.h"

#include <cstdlib>
#include <glm/gtc/constants.hpp>
#include <QuasarEngine/Renderer/RenderContext.h>

namespace QuasarEngine
{
    ParticleComponent::ParticleComponent()
    {
        
    }

    void ParticleComponent::SetTexture(const std::string& assetId, const std::string& absPath)
    {
        m_TextureId = assetId;
        m_TexturePath = absPath;
        RebuildSystem();
    }

    void ParticleComponent::ClearTexture()
    {
        m_TextureId.clear();
        m_TexturePath.clear();
        m_System.reset();
    }

    void ParticleComponent::RebuildSystem()
    {
        if (m_TexturePath.empty() || m_MaxParticles <= 0)
        {
            m_System.reset();
            return;
        }

        m_MaxParticles = std::max(1, m_MaxParticles);
        m_System = std::make_unique<ParticleSystem>(m_TexturePath, m_MaxParticles);
    }

    float ParticleComponent::Random01()
    {
        return static_cast<float>(std::rand()) / static_cast<float>(RAND_MAX);
    }

    float ParticleComponent::RandomRange(float min, float max)
    {
        return min + (max - min) * Random01();
    }

    void ParticleComponent::EmitOne(const glm::vec3& emitterPosition)
    {
        if (!m_System)
            return;

        glm::vec3 pos = emitterPosition;
        glm::vec3 vel = m_BaseVelocity;

        if (m_PositionSpread > 0.0f)
        {
            pos.x += RandomRange(-m_PositionSpread, m_PositionSpread);
            pos.z += RandomRange(-m_PositionSpread, m_PositionSpread);
        }

        if (m_VelocitySpread > 0.0f)
        {
            vel.x += RandomRange(-m_VelocitySpread, m_VelocitySpread);
            vel.z += RandomRange(-m_VelocitySpread, m_VelocitySpread);
        }

        float endSize = m_EndSize;
        if (m_EndSizeJitter > 0.0f)
        {
            endSize += RandomRange(-m_EndSizeJitter, m_EndSizeJitter);
        }
        if (endSize < 0.1f)
            endSize = 0.1f;

        float rotation = RandomRange(0.0f, glm::two_pi<float>());

        m_System->Emit(
            pos,
            vel,
            m_ParticleLife,
            m_StartSize,
            endSize,
            m_ColorStart,
            m_ColorEnd,
            rotation
        );
    }

    void ParticleComponent::Update(float dt, const glm::vec3& worldEmitterPos)
    {
        if (!m_System)
            return;

        if (!m_Playing)
        {
            m_System->Update(dt);
            return;
        }

        m_SpawnTimer += dt;

        if (m_SpawnInterval <= 0.0f)
            m_SpawnInterval = 0.01f;

        while (m_SpawnTimer >= m_SpawnInterval)
        {
            m_SpawnTimer -= m_SpawnInterval;

            EmitOne(worldEmitterPos);
        }

        m_System->Update(dt);
    }

    void ParticleComponent::Render(RenderContext& ctx)
    {
        if (!m_System)
            return;

        m_System->Render(ctx);
    }
}
