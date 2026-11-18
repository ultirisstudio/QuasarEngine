#include <quasar/renderer/Mesh.h>

#include <quasar/log/Log.h>

#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <span>
#include <utility>

namespace Quasar
{
    VertexLayout Vertex::GetLayout()
    {
        return {{0, VertexAttributeType::Vec3}, {1, VertexAttributeType::Vec3}, {2, VertexAttributeType::Vec2}};
    }

    Mesh::Mesh(Scope<VertexArray> vertexArray) : m_VertexArray(std::move(vertexArray)) {}

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
        return Scope<Mesh>(new Mesh(std::move(vertexArray)));
    }
}
