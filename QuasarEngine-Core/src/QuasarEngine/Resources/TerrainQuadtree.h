#pragma once

#include <memory>
#include <vector>
#include <functional>
#include <limits>
#include <glm/glm.hpp>

#include <QuasarEngine/Resources/Mesh.h>
#include <QuasarEngine/Renderer/Buffer.h>
#include <QuasarEngine/Renderer/DrawMode.h>

namespace QuasarEngine
{
    struct TerrainLODSettings
    {
        int maxDepth = 5;
        int leafNodeSize = 16;

        std::vector<float> lodDistances;

        TerrainLODSettings()
        {
            lodDistances.resize(maxDepth + 1);
            for (int d = 0; d <= maxDepth; ++d)
                lodDistances[d] = 2000.0f / float(1 << d);
        }
    };

    class TerrainQuadtree
    {
    public:
        using HeightSampler = std::function<float(int gx, int gz)>;

        struct Node
        {
            glm::ivec2 origin;
            int size;
            int depth;

            glm::vec3 bboxMin;
            glm::vec3 bboxMax;

            std::shared_ptr<Mesh> mesh;

            std::unique_ptr<Node> children[4];

            bool IsLeaf() const { return children[0] == nullptr; }
        };

        TerrainQuadtree(int gridW,
            int gridH,
            float worldSizeX,
            float worldSizeZ,
            float heightScale,
            HeightSampler sampler,
            const TerrainLODSettings& lodSettings);

        void Build();

        void CollectVisible(const glm::mat4& viewProj,
            const glm::vec3& cameraPos,
            std::vector<const Node*>& outNodes) const;

        const Node* GetRoot() const { return m_root.get(); }
        const TerrainLODSettings& GetLODSettings() const { return m_lod; }

    private:
        struct Frustum
        {
            glm::vec4 planes[6];
        };

        Frustum ExtractFrustum(const glm::mat4& viewProj) const;
        static bool BoxInFrustum(const Frustum& f,
            const glm::vec3& bmin,
            const glm::vec3& bmax);

        bool ShouldSubdivide(const Node& node,
            const glm::vec3& cameraPos) const;

        std::unique_ptr<Node> BuildNode(const glm::ivec2& origin,
            int size,
            int depth);

        void CollectVisibleRecursive(const Node* node,
            const Frustum& frustum,
            const glm::vec3& cameraPos,
            std::vector<const Node*>& outNodes) const;

        float      SampleHeight01(int gx, int gz) const;
        glm::vec3  ComputeNormal(int gx, int gz) const;

        std::shared_ptr<Mesh> BuildLeafMesh(const glm::ivec2& origin,
            int size,
            glm::vec3& outBMin,
            glm::vec3& outBMax) const;

    private:
        int   m_gridW = 0;
        int   m_gridH = 0;
        float m_worldSizeX = 1.0f;
        float m_worldSizeZ = 1.0f;
        float m_heightScale = 1.0f;

        float m_cellSizeX = 1.0f;
        float m_cellSizeZ = 1.0f;

        HeightSampler      m_sampler;
        TerrainLODSettings m_lod;

        std::unique_ptr<Node> m_root;
    };
}