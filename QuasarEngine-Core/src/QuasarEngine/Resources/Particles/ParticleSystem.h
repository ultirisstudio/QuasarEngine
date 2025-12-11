#pragma once

#include <memory>
#include <string>
#include <vector>

#include <glm/glm.hpp>

#include <QuasarEngine/Renderer/DrawMode.h>
#include <QuasarEngine/Renderer/RenderContext.h>
#include <QuasarEngine/Renderer/VertexArray.h>
#include <QuasarEngine/Renderer/Buffer.h>
#include <QuasarEngine/Shader/Shader.h>
#include <QuasarEngine/Resources/Texture2D.h>

namespace QuasarEngine
{
    struct Particle
    {
        glm::vec3 position{ 0.0f };
        glm::vec3 velocity{ 0.0f };

        float size = 1.0f;
        float startSize = 1.0f;
        float endSize = 1.0f;

        float age = 0.0f;
        float lifetime = 1.0f;

        glm::vec4 colorStart{ 1.0f, 1.0f, 1.0f, 1.0f };
        glm::vec4 colorEnd{ 1.0f, 1.0f, 1.0f, 0.0f };

        float rotation = 0.0f;
        float angularVelocity = 0.0f;

        float random = 0.0f;
    };

    class ParticleSystem
    {
    public:
        struct SimulationSettings
        {
            glm::vec3 gravity{ 0.0f, 1.5f, 0.0f };
            float linearDrag = 1.0f;

            glm::vec3 wind{ 0.0f, 0.0f, 0.0f };
            float turbulenceStrength = 0.0f;
            float turbulenceFrequency = 1.0f;
            float turbulenceScale = 1.0f;

            float sizeOverLifeExponent = 1.0f;
            float alphaOverLifeExponent = 1.0f;
        };

    public:
        ParticleSystem(const std::string& texturePath, std::size_t maxParticles = 512);
        ~ParticleSystem() = default;

        void Emit(const Particle& spawnData);

        void Update(float dt);
        void Render(RenderContext& ctx);

        void SetSimulationSettings(const SimulationSettings& s) { m_Settings = s; }
        const SimulationSettings& GetSimulationSettings() const { return m_Settings; }

        std::size_t GetMaxParticles() const { return m_MaxParticles; }

        void SetTexture(const std::string& texturePath);

    private:
        void InitMesh();
        void InitShader(const std::string& basePath);

    private:
        std::vector<Particle> m_Particles;
        std::size_t m_MaxParticles = 512;

        std::vector<std::size_t> m_AliveIndices;
        std::vector<float> m_AliveDistances;
        
        struct GPUParticle
        {
            glm::vec3 position;
            float size;

            glm::vec4 colorStart;
            glm::vec4 colorEnd;

            float age;
            float lifetime;
            float rotation;
            float random;
        };
        std::vector<GPUParticle> m_GPUBuffer;

        std::shared_ptr<VertexArray> m_VertexArray;
        std::shared_ptr<VertexBuffer> m_VertexBuffer;
        std::shared_ptr<IndexBuffer> m_IndexBuffer;

        DrawMode m_DrawMode = DrawMode::TRIANGLES;
        std::size_t m_VertexCount = 0;
        std::size_t m_IndexCount = 0;

        std::shared_ptr<Shader> m_Shader;
        std::shared_ptr<Texture2D> m_Texture;

        SimulationSettings m_Settings;

        float m_Time = 0.0f;
    };
}
