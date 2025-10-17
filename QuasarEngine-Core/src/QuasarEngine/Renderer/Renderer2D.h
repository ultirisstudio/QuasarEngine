#pragma once

#include <vector>
#include <memory>
#include <glm/glm.hpp>

#include <QuasarEngine/Core/Singleton.h>
#include <QuasarEngine/Shader/Shader.h>

namespace QuasarEngine
{
    class BaseCamera;
    class Texture;
    class Texture2D;

    struct Quad2D
    {
        glm::mat4 transform{ 1.0f };
        glm::vec4 color{ 1.0f };
        glm::vec4 uv{ 0.0f, 0.0f, 1.0f, 1.0f };
        glm::vec2 tiling{ 1.0f, 1.0f };
        glm::vec2 offset{ 0.0f, 0.0f };
        int        order = 0;
        Texture* texture = nullptr;
    };

    class Renderer2D : public Singleton<Renderer2D>
    {
    public:
        void Initialize();
        void Shutdown();

        void BeginScene(const BaseCamera& camera);
        
        void Submit(const Quad2D& q);
        void EndScene();

        void DrawQuad(const glm::mat4& transform,
            Texture* tex,
            const glm::vec4& color = glm::vec4(1.0f),
            const glm::vec4& uv = { 0,0,1,1 },
            const glm::vec2& tiling = { 1,1 },
            const glm::vec2& offset = { 0,0 },
            int order = 0)
        {
            Quad2D q; q.transform = transform; q.texture = tex;
            q.color = color; q.uv = uv; q.tiling = tiling; q.offset = offset; q.order = order;
            Submit(q);
        }

        struct Stats { uint32_t drawCalls = 0; uint32_t quadCount = 0; };
        const Stats& GetStats() const noexcept { return m_Stats; }
        void ResetStats() noexcept { m_Stats = {}; }

        std::shared_ptr<Shader> GetShader() const noexcept { return m_Shader; }

    private:
        void Flush();

        struct QuadVertex {
            glm::vec3 pos;
            glm::vec4 color;
            glm::vec2 uv;
            glm::vec2 tiling;
            glm::vec2 offset;
        };

        struct DrawCmd {
            Texture* texture;
            int        order;
            glm::mat4  transform;
            glm::vec4  color;
            glm::vec4  uv;
            glm::vec2  tiling;
            glm::vec2  offset;
        };

        static void PushQuadCPU(std::vector<QuadVertex>& vv, std::vector<uint32_t>& ii, const DrawCmd& c);

        std::vector<QuadVertex> m_CPUVertices;
        std::vector<uint32_t>   m_CPUIndices;

        std::vector<DrawCmd>    m_Queue;

        std::shared_ptr<Shader> m_Shader;
        
        std::shared_ptr<class VertexArray>  m_VAO;
        std::shared_ptr<class VertexBuffer> m_VBO;
        std::shared_ptr<class IndexBuffer>  m_IBO;

        glm::mat4 m_View{ 1.0f };
        glm::mat4 m_Proj{ 1.0f };

        static constexpr uint32_t kMaxQuads = 10'000;
        static constexpr uint32_t kMaxVertices = kMaxQuads * 4;
        static constexpr uint32_t kMaxIndices = kMaxQuads * 6;

        Stats m_Stats{};
    };
}