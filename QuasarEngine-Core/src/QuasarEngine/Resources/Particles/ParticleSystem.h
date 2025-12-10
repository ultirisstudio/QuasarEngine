#pragma once

#include <memory>
#include <string>
#include <vector>

#include <glm/glm.hpp>

#include <QuasarEngine/Renderer/DrawMode.h>
#include <QuasarEngine/Renderer/RenderContext.h>
#include <QuasarEngine/Resources/Mesh.h>
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

        float life = 0.0f;
        float maxLife = 1.0f;

        glm::vec4 colorStart{ 1.0f, 1.0f, 1.0f, 1.0f };
        glm::vec4 colorEnd{ 1.0f, 1.0f, 1.0f, 0.0f };

        float rotation = 0.0f;
    };

    class ParticleSystem
    {
    public:
        ParticleSystem(const std::string& texturePath, std::size_t maxParticles = 128);
        ~ParticleSystem() = default;

        void Emit(const glm::vec3& position,
            const glm::vec3& velocity,
            float life,
            float startSize,
            float endSize,
            const glm::vec4& colorStart,
            const glm::vec4& colorEnd,
            float rotation);

        void Update(float dt);
        void Render(RenderContext& ctx);

    private:
        glm::mat4 BuildBillboard(const glm::mat4& view,
            const glm::vec3& position,
            float size,
            float rotation);

        void InitMesh();
        void InitShader(const std::string& basePath);

    private:
        std::vector<Particle> m_Particles;
        std::size_t m_MaxParticles = 128;

        std::shared_ptr<Mesh> m_QuadMesh;
        std::shared_ptr<Shader> m_Shader;
        std::shared_ptr<Texture2D> m_Texture;
    };
}
