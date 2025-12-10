#pragma once

#include <cstdlib>
#include <glm/glm.hpp>
#include <glm/gtc/constants.hpp>
#include <QuasarEngine/Resources/Particles/ParticleSystem.h>

namespace QuasarEngine
{
    class RenderContext;

    class SmokeEmitterScript
    {
    public:
        SmokeEmitterScript()
            : m_Particles("Assets/Textures/puff.png", 64)
        {
            m_EmitterPosition = glm::vec3(0.0f, 0.0f, 0.0f);

            m_BaseVelocity = glm::vec3(0.0f, 0.7f, 0.0f);
            m_ParticleLife = 4.0f;
            m_StartSize = 0.4f;
            m_EndSize = 1.5f;

            m_SpawnInterval = 0.25f;
            m_SpawnTimer = 0.0f;
        }

        void OnUpdate(float dt)
        {
            m_SpawnTimer += dt;

            while (m_SpawnTimer >= m_SpawnInterval)
            {
                m_SpawnTimer -= m_SpawnInterval;

                glm::vec3 pos = m_EmitterPosition;
                glm::vec3 vel = m_BaseVelocity;

                float posSpread = 0.1f;
                pos.x += RandomRange(-posSpread, posSpread);
                pos.z += RandomRange(-posSpread, posSpread);

                float velSpread = 0.2f;
                vel.x += RandomRange(-velSpread, velSpread);
                vel.z += RandomRange(-velSpread, velSpread);

                float endSizeJitter = 0.15f;
                float endSize = m_EndSize + RandomRange(-endSizeJitter, endSizeJitter);
                if (endSize < 0.1f) endSize = 0.1f;

                float rotation = RandomRange(0.0f, glm::two_pi<float>());

                glm::vec4 colorStart(1.0f);
                glm::vec4 colorEnd(1.0f, 1.0f, 1.0f, 0.0f);

                m_Particles.Emit(
                    pos,
                    vel,
                    m_ParticleLife,
                    m_StartSize,
                    endSize,
                    colorStart,
                    colorEnd,
                    rotation
                );
            }

            m_Particles.Update(dt);
        }

        void OnRender(RenderContext& ctx)
        {
            m_Particles.Render(ctx);
        }

    private:
        static float Random01()
        {
            return static_cast<float>(std::rand()) / static_cast<float>(RAND_MAX);
        }

        static float RandomRange(float min, float max)
        {
            return min + (max - min) * Random01();
        }

    private:
        ParticleSystem m_Particles;

        glm::vec3 m_EmitterPosition{ 0.0f, 0.0f, 0.0f };
        glm::vec3 m_BaseVelocity{ 0.0f, 0.7f, 0.0f };

        float m_ParticleLife = 4.0f;
        float m_StartSize = 0.4f;
        float m_EndSize = 0.7f;

        float m_SpawnInterval = 0.25f;
        float m_SpawnTimer = 0.0f;
    };
}