#include <quasar/renderer/Mesh.h>

#include <quasar/log/Log.h>

#include <glm/geometric.hpp>
#include <glm/vec2.hpp>
#include <glm/vec3.hpp>
#include <glm/vec4.hpp>

#include <algorithm>
#include <array>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <cstring>
#include <limits>
#include <optional>
#include <span>
#include <utility>
#include <vector>

namespace Quasar
{
    namespace
    {
        std::optional<BoundingBox> ComputeBounds(std::span<const std::byte> vertices, const VertexLayout& layout)
        {
            const std::vector<VertexAttribute>& attributes = layout.GetAttributes();
            const auto position = std::ranges::find_if(attributes, [](const VertexAttribute& attribute)
                                                       { return attribute.Location == 0 && attribute.Type == VertexAttributeType::Vec3; });
            if (position == attributes.end())
            {
                return std::nullopt;
            }

            const std::size_t offset = layout.GetOffset(static_cast<std::size_t>(position - attributes.begin()));
            const std::size_t stride = layout.GetStride();
            std::vector<glm::vec3> points(vertices.size() / stride);
            for (std::size_t index = 0; index < points.size(); ++index)
            {
                std::memcpy(&points[index], vertices.subspan((index * stride) + offset, sizeof(glm::vec3)).data(), sizeof(glm::vec3));
            }
            return BoundingBox::FromPoints(points);
        }
    }

    VertexLayout Vertex::GetLayout()
    {
        return {{0, VertexAttributeType::Vec3}, {1, VertexAttributeType::Vec3}, {2, VertexAttributeType::Vec2}, {3, VertexAttributeType::Vec4}};
    }

    void MeshData::GenerateTangents()
    {
        std::vector<glm::vec3> towardU(Vertices.size(), glm::vec3(0.0f));
        std::vector<glm::vec3> towardV(Vertices.size(), glm::vec3(0.0f));
        for (std::size_t first = 0; first + 2 < Indices.size(); first += 3)
        {
            const std::array<std::uint32_t, 3> triangle{Indices[first], Indices[first + 1], Indices[first + 2]};
            if (std::ranges::any_of(triangle, [this](std::uint32_t index) { return index >= Vertices.size(); }))
            {
                continue;
            }
            const Vertex& a = Vertices[triangle[0]];
            const Vertex& b = Vertices[triangle[1]];
            const Vertex& c = Vertices[triangle[2]];
            const glm::vec3 edge1 = b.Position - a.Position;
            const glm::vec3 edge2 = c.Position - a.Position;
            const glm::vec2 uv1 = b.TexCoord - a.TexCoord;
            const glm::vec2 uv2 = c.TexCoord - a.TexCoord;
            const float determinant = (uv1.x * uv2.y) - (uv2.x * uv1.y);
            if (std::abs(determinant) < std::numeric_limits<float>::epsilon())
            {
                continue;
            }
            const glm::vec3 u = ((edge1 * uv2.y) - (edge2 * uv1.y)) / determinant;
            const glm::vec3 v = ((edge2 * uv1.x) - (edge1 * uv2.x)) / determinant;
            for (const std::uint32_t index : triangle)
            {
                towardU[index] += u;
                towardV[index] += v;
            }
        }

        for (std::size_t index = 0; index < Vertices.size(); ++index)
        {
            Vertex& vertex = Vertices[index];
            glm::vec3 tangent = towardU[index] - (vertex.Normal * glm::dot(vertex.Normal, towardU[index]));
            if (glm::dot(tangent, tangent) < std::numeric_limits<float>::epsilon())
            {
                const glm::vec3 axis = std::abs(vertex.Normal.x) < 0.9f ? glm::vec3(1.0f, 0.0f, 0.0f) : glm::vec3(0.0f, 1.0f, 0.0f);
                tangent = glm::cross(axis, vertex.Normal);
                if (glm::dot(tangent, tangent) < std::numeric_limits<float>::epsilon())
                {
                    tangent = axis;
                }
            }
            const float handedness = glm::dot(glm::cross(vertex.Normal, tangent), -towardV[index]) < 0.0f ? -1.0f : 1.0f;
            vertex.Tangent = glm::vec4(glm::normalize(tangent), handedness);
        }
    }

    Mesh::Mesh(Scope<VertexArray> vertexArray, const std::optional<BoundingBox>& bounds) : m_VertexArray(std::move(vertexArray)), m_Bounds(bounds) {}

    Scope<Mesh> Mesh::Create(GraphicsContext& context, const MeshData& data)
    {
        return Create(context, std::as_bytes(std::span(data.Vertices)), Vertex::GetLayout(), data.Indices);
    }

    Scope<Mesh> Mesh::Create(GraphicsContext& context, std::span<const std::byte> vertices, const VertexLayout& layout, std::span<const std::uint32_t> indices)
    {
        if (indices.empty() || indices.size() % 3 != 0)
        {
            QUASAR_ENGINE_ERROR("Cannot create a mesh of {} indices: it needs 3 indices per triangle", indices.size());
            return nullptr;
        }

        Scope<VertexBuffer> vertexBuffer = context.CreateVertexBuffer(vertices, layout);
        if (vertexBuffer == nullptr)
        {
            return nullptr;
        }
        const std::uint32_t vertexCount = vertexBuffer->GetVertexCount();
        if (const std::uint32_t largest = std::ranges::max(indices); largest >= vertexCount)
        {
            QUASAR_ENGINE_ERROR("Cannot create a mesh whose triangles use the vertex {}: it has {} vertices", largest, vertexCount);
            return nullptr;
        }

        Scope<IndexBuffer> indexBuffer = context.CreateIndexBuffer(indices);
        Scope<VertexArray> vertexArray = context.CreateVertexArray();
        if (indexBuffer == nullptr || vertexArray == nullptr)
        {
            return nullptr;
        }
        vertexArray->AddVertexBuffer(std::move(vertexBuffer));
        vertexArray->SetIndexBuffer(std::move(indexBuffer));
        return Scope<Mesh>(new Mesh(std::move(vertexArray), ComputeBounds(vertices, layout)));
    }
}
