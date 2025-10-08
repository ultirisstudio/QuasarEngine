#pragma once

#include <vector>
#include <optional>
#include <memory>

#include <glm/glm.hpp>

#include <QuasarEngine/Asset/Asset.h>
#include <QuasarEngine/Resources/Materials/Material.h>
#include <QuasarEngine/Tools/Math.h>
#include <QuasarEngine/Renderer/VertexArray.h>
#include <QuasarEngine/Renderer/Buffer.h>
#include <QuasarEngine/Renderer/DrawMode.h>

namespace QuasarEngine
{
    class Mesh : public Asset
    {
    public:
        Mesh(std::vector<float> vertices,
            std::vector<unsigned int> indices,
            std::optional<BufferLayout> layout = std::nullopt,
            DrawMode drawMode = DrawMode::TRIANGLES,
            std::optional<MaterialSpecification> material = std::nullopt);
        ~Mesh();

        void draw() const;

        void SetSkinning(const std::vector<int>& boneIDs,
            const std::vector<float>& boneWeights,
            int maxInfluencesPerVertex);

        std::optional<MaterialSpecification> GetMaterial() const { return m_material; }

        void GenerateMesh(std::vector<float> vertices,
            std::vector<unsigned int> indices,
            std::optional<BufferLayout> layout = std::nullopt);

        glm::vec3 GetBoundingBoxSize() const { return m_boundingBoxSize; }
        bool IsVisible(const Math::Frustum& frustum, const glm::mat4& modelMatrix) const;
        bool IsMeshGenerated() const { return m_meshGenerated; }

        void Clear();

        const size_t& GetVerticesCount() const { return m_vertices.size(); }
        const size_t& GetIndicesCount() const { return m_indices.size(); }
        const std::vector<float>& GetVertices() const { return m_vertices; }
        const std::vector<unsigned int>& GetIndices() const { return m_indices; }

        static AssetType GetStaticType() { return AssetType::MESH; }
        AssetType GetType() override { return GetStaticType(); }

    private:
        void CalculateBoundingBoxSize(const std::vector<float>& vertices, uint32_t strideFloats = 8u);

    private:
        std::shared_ptr<VertexArray> m_vertexArray;
        std::shared_ptr<VertexBuffer> m_vertexBuffer;
        std::shared_ptr<VertexBuffer> m_boneIdBuffer;
        std::shared_ptr<VertexBuffer> m_boneWeightBuffer;
        std::shared_ptr<IndexBuffer>  m_indexBuffer;

        DrawMode m_drawMode = DrawMode::TRIANGLES;

        std::vector<float> m_vertices;
        std::vector<unsigned int> m_indices;

        glm::vec3 m_boundingBoxSize{ 0 };
        glm::vec3 m_boundingBoxPosition{ 0 };

        bool m_meshGenerated = false;

        std::optional<MaterialSpecification> m_material;
    };
}