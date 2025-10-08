#include "qepch.h"

#include <limits>
#include <glm/gtx/matrix_decompose.hpp>

#include <QuasarEngine/Resources/Mesh.h>
#include <QuasarEngine/Renderer/RenderCommand.h>

namespace QuasarEngine
{
    void Mesh::CalculateBoundingBoxSize(const std::vector<float>& vertices, uint32_t strideFloats)
    {
        float minX = std::numeric_limits<float>::max();
        float minY = std::numeric_limits<float>::max();
        float minZ = std::numeric_limits<float>::max();
        float maxX = -std::numeric_limits<float>::max();
        float maxY = -std::numeric_limits<float>::max();
        float maxZ = -std::numeric_limits<float>::max();

        if (strideFloats < 3) return;
        for (size_t i = 0; i + 2 < vertices.size(); i += strideFloats)
        {
            minX = std::min(minX, vertices[i + 0]);
            minY = std::min(minY, vertices[i + 1]);
            minZ = std::min(minZ, vertices[i + 2]);
            maxX = std::max(maxX, vertices[i + 0]);
            maxY = std::max(maxY, vertices[i + 1]);
            maxZ = std::max(maxZ, vertices[i + 2]);
        }
        m_boundingBoxSize = glm::vec3(maxX - minX, maxY - minY, maxZ - minZ);
        m_boundingBoxPosition = glm::vec3(minX, minY, minZ);
    }

    Mesh::Mesh(std::vector<float> vertices,
        std::vector<unsigned int> indices,
        std::optional<BufferLayout> layout,
        DrawMode drawMode,
        std::optional<MaterialSpecification> material)
        : m_drawMode(drawMode), m_vertices(vertices), m_indices(indices), m_material(material)
    {
        GenerateMesh(std::move(vertices), std::move(indices), std::move(layout));
    }

    Mesh::~Mesh()
    {
        m_boneWeightBuffer.reset();
        m_boneIdBuffer.reset();
        m_vertexBuffer.reset();
        m_indexBuffer.reset();
        m_vertexArray.reset();
    }

    void Mesh::draw() const
    {
        if (!m_vertexArray) return;
        m_vertexArray->Bind();
        const uint32_t count = m_indexBuffer ? m_indexBuffer->GetCount() : 0u;
        if (count == 0)
            RenderCommand::Instance().DrawArrays(m_drawMode, static_cast<uint32_t>(m_vertices.size() / 8));
        else
            RenderCommand::Instance().DrawElements(m_drawMode, count);
    }

    void Mesh::GenerateMesh(std::vector<float> vertices,
        std::vector<unsigned int> indices,
        std::optional<BufferLayout> layout)
    {
        m_vertexArray = VertexArray::Create();

        m_vertexBuffer = VertexBuffer::Create(vertices.data(), static_cast<uint32_t>(vertices.size() * sizeof(float)));
        if (layout.has_value()) m_vertexBuffer->SetLayout(layout.value());
        else {
            m_vertexBuffer->SetLayout({
                { ShaderDataType::Vec3, "inPosition" },
                { ShaderDataType::Vec3, "inNormal"   },
                { ShaderDataType::Vec2, "inTexCoord" }
                });
        }
        m_vertexArray->AddVertexBuffer(m_vertexBuffer);

        m_indexBuffer = IndexBuffer::Create(indices.data(), static_cast<uint32_t>(indices.size() * sizeof(unsigned int)));
        m_vertexArray->SetIndexBuffer(m_indexBuffer);

        m_vertices = std::move(vertices);
        m_indices = std::move(indices);

        const uint32_t strideFloats = static_cast<uint32_t>(m_vertexBuffer->GetLayout().GetStride() / sizeof(float));
        CalculateBoundingBoxSize(m_vertices, strideFloats ? strideFloats : 8u);

        m_meshGenerated = true;
    }

    void Mesh::SetSkinning(const std::vector<int>& boneIDs,
        const std::vector<float>& boneWeights,
        int maxInfluencesPerVertex)
    {
        if (boneIDs.empty() || boneWeights.empty()) return;

        m_boneIdBuffer = VertexBuffer::Create(boneIDs.data(), static_cast<uint32_t>(boneIDs.size() * sizeof(int)));
        m_boneIdBuffer->SetLayout({ { ShaderDataType::IVec4, "inBoneIDs" } });
        m_vertexArray->AddVertexBuffer(m_boneIdBuffer);

        m_boneWeightBuffer = VertexBuffer::Create(boneWeights.data(), static_cast<uint32_t>(boneWeights.size() * sizeof(float)));
        m_boneWeightBuffer->SetLayout({ { ShaderDataType::Vec4, "inWeights" } });
        m_vertexArray->AddVertexBuffer(m_boneWeightBuffer);

		m_hasSkinning = true;

        (void)maxInfluencesPerVertex;
    }

    bool Mesh::IsVisible(const Math::Frustum& frustum, const glm::mat4& modelMatrix) const
    {
        const glm::vec3 bbMinLocal = m_boundingBoxPosition;
        const glm::vec3 bbSizeLocal = m_boundingBoxSize;
        const glm::vec3 bbCenterL = bbMinLocal + 0.5f * bbSizeLocal;
        const glm::vec3 bbHalfL = 0.5f * bbSizeLocal;

        const glm::vec3 centerW = glm::vec3(modelMatrix * glm::vec4(bbCenterL, 1.0f));
        const glm::vec3 axisX = glm::vec3(modelMatrix[0]) * bbHalfL.x;
        const glm::vec3 axisY = glm::vec3(modelMatrix[1]) * bbHalfL.y;
        const glm::vec3 axisZ = glm::vec3(modelMatrix[2]) * bbHalfL.z;

        for (int i = 0; i < 6; ++i)
        {
            const glm::vec3 n = frustum.planes[i].n;
            const float d = frustum.planes[i].d;
            const float dist = glm::dot(n, centerW) + d;
            const float r = std::abs(glm::dot(n, axisX)) + std::abs(glm::dot(n, axisY)) + std::abs(glm::dot(n, axisZ));
            if (dist + r < 0.0f) return false;
        }
        return true;
    }

    void Mesh::Clear()
    {
        m_vertices.clear();
        m_indices.clear();
        m_meshGenerated = false;
    }
}
