#include "qepch.h"
#include "ParticleComponent.h"

#include <cstdlib>
#include <glm/gtc/constants.hpp>

#include <QuasarEngine/Renderer/RenderContext.h>

#include <QuasarEngine/Entity/Entity.h>
#include <QuasarEngine/Entity/Components/TransformComponent.h>

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
        if (m_TexturePath.empty())
        {
            m_System.reset();
            return;
        }

        m_System = std::make_unique<ParticleSystem>(m_TexturePath,
            static_cast<std::size_t>(m_MaxParticles));

        if (!m_System)
            return;

        ParticleSystem::SimulationSettings s;
        s.gravity = m_Gravity;
        s.linearDrag = m_LinearDrag;
        s.wind = m_Wind;
        s.turbulenceStrength = m_TurbulenceStrength;
        s.turbulenceFrequency = m_TurbulenceFrequency;
        s.turbulenceScale = m_TurbulenceScale;
        s.sizeOverLifeExponent = m_SizeOverLifeExponent;
        s.alphaOverLifeExponent = m_AlphaOverLifeExponent;

        m_System->SetSimulationSettings(s);
    }

    float ParticleComponent::Random01()
    {
        return static_cast<float>(std::rand()) / static_cast<float>(RAND_MAX);
    }

    float ParticleComponent::RandomRange(float min, float max)
    {
        return min + (max - min) * Random01();
    }

    void ParticleComponent::EmitOne(const glm::vec3& emitterWorldPos)
    {
        if (!m_System)
            return;

        Particle p{};

        p.position = emitterWorldPos + m_EmitterOffset;

        if (m_PositionSpread > 0.0f)
        {
            float u = Random01();
            float v = Random01();
            float theta = 2.0f * glm::pi<float>() * u;
            float phi = std::acos(2.0f * v - 1.0f);
            float r = Random01() * m_PositionSpread;

            glm::vec3 offset(
                r * std::sin(phi) * std::cos(theta),
                r * std::cos(phi),
                r * std::sin(phi) * std::sin(theta)
            );
            p.position += offset;
        }

        glm::vec3 dir = m_BaseDirection;
        if (glm::length2(dir) < 1e-4f)
            dir = glm::vec3(0.0f, 1.0f, 0.0f);
        dir = glm::normalize(dir);

        float spreadRad = glm::radians(m_SpreadAngleDegrees);
        float angle = RandomRange(0.0f, spreadRad);
        float azimuth = RandomRange(0.0f, glm::two_pi<float>());

        glm::vec3 up = (std::abs(dir.y) < 0.99f)
            ? glm::vec3(0.0f, 1.0f, 0.0f)
            : glm::vec3(1.0f, 0.0f, 0.0f);

        glm::vec3 right = glm::normalize(glm::cross(up, dir));
        glm::vec3 forward = glm::normalize(glm::cross(dir, right));

        glm::vec3 offsetDir =
            glm::normalize(
                dir * std::cos(angle) +
                (right * std::cos(azimuth) + forward * std::sin(azimuth)) * std::sin(angle)
            );

        float speed = RandomRange(m_SpeedMin, m_SpeedMax);
        p.velocity = offsetDir * speed;

        if (m_VelocitySpread > 0.0f)
        {
            float factor = 1.0f + (Random01() * 2.0f - 1.0f) * m_VelocitySpread;
            p.velocity *= factor;
        }

        p.age = 0.0f;
        p.lifetime = RandomRange(m_LifeMin, m_LifeMax);

        p.startSize = RandomRange(m_StartSizeMin, m_StartSizeMax);
        p.endSize = RandomRange(m_EndSizeMin, m_EndSizeMax);
        p.size = p.startSize;

        if (m_RandomRotation)
            p.rotation = RandomRange(0.0f, glm::two_pi<float>());
        else
            p.rotation = 0.0f;

        p.angularVelocity = RandomRange(m_AngularVelocityMin, m_AngularVelocityMax);

        p.colorStart = m_ColorStart;
        p.colorEnd = m_ColorEnd;

        p.random = Random01();

        m_System->Emit(p);
    }

    void ParticleComponent::Update(float dt)
    {
        if (!m_System || !m_Enabled)
            return;

        if (m_OverrideSimulation)
        {
            ParticleSystem::SimulationSettings s;
            s.gravity = m_Gravity;
            s.linearDrag = m_LinearDrag;
            s.wind = m_Wind;
            s.turbulenceStrength = m_TurbulenceStrength;
            s.turbulenceFrequency = m_TurbulenceFrequency;
            s.turbulenceScale = m_TurbulenceScale;
            s.sizeOverLifeExponent = m_SizeOverLifeExponent;
            s.alphaOverLifeExponent = m_AlphaOverLifeExponent;

            m_System->SetSimulationSettings(s);
        }

		Entity owner{ entt_entity, registry };
        glm::vec3 worldEmitterPos = owner.GetComponent<TransformComponent>().Position;
        worldEmitterPos += m_EmitterOffset;

        if (m_Emitting && m_SpawnRate > 0.0f)
        {
            float interval = 1.0f / m_SpawnRate;
            m_SpawnTimer += dt;

            while (m_SpawnTimer >= interval)
            {
                m_SpawnTimer -= interval;

                int count = m_BurstMode ? std::max(1, m_BurstCount) : 1;
                for (int i = 0; i < count; ++i)
                    EmitOne(worldEmitterPos);

                if (m_BurstMode && !m_Loop)
                {
                    m_Emitting = false;
                    break;
                }
            }
        }

        m_System->Update(dt);
    }

    void ParticleComponent::Render(RenderContext& ctx)
    {
        if (!m_System || !m_Enabled)
            return;

        m_System->Render(ctx);
    }
}
