#include <quasar/renderer/Mesh.h>

#include <quasar/log/Log.h>

#include <glm/vec3.hpp>

#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <cstring>
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
        return {{0, VertexAttributeType::Vec3}, {1, VertexAttributeType::Vec3}, {2, VertexAttributeType::Vec2}};
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
