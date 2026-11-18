#pragma once

/// \file
/// \brief Meshes: the triangles of an object, stored on the graphics card and ready to draw.
/// \author Kurogenshi
/// \date 2026-11-18

#include <quasar/core/Base.h>
#include <quasar/renderer/GraphicsContext.h>
#include <quasar/renderer/VertexArray.h>
#include <quasar/renderer/VertexLayout.h>

#include <glm/vec2.hpp>
#include <glm/vec3.hpp>

#include <cstddef>
#include <cstdint>
#include <span>
#include <vector>

namespace Quasar
{
    /// @brief The standard vertex of the meshes, read by a vertex shader with `layout(location = 0) in vec3
    /// a_Position;`, `layout(location = 1) in vec3 a_Normal;` and `layout(location = 2) in vec2 a_TexCoord;`.
    struct Vertex
    {
        glm::vec3 Position{0.0f}; ///< The position, in the space of the mesh.
        glm::vec3 Normal{0.0f};   ///< The direction the surface faces, of length 1.
        glm::vec2 TexCoord{0.0f}; ///< The texture coordinates, with v = 0 at the top of the image.

        /// @brief Returns the layout of this vertex.
        static VertexLayout GetLayout();
    };

    /// @brief The vertices and the triangles of a mesh, in memory, before they are sent to the graphics card.
    ///
    /// Each 3 indices give one triangle, whose front is the side from which its vertices turn counter-clockwise.
    struct MeshData
    {
        std::vector<Vertex> Vertices;       ///< The vertices.
        std::vector<std::uint32_t> Indices; ///< The indices of the vertices of each triangle.
    };

    /// @brief The vertex array of a mesh, drawn with `GraphicsContext::DrawIndexed(mesh.GetVertexArray())`.
    class Mesh
    {
    public:
        /// @brief Sends a mesh of standard vertices to the graphics card.
        /// @return The mesh, or nullptr if it has no triangles, if an index is not a vertex, or if the number of
        /// indices is not a multiple of 3. The reason is logged as an engine error.
        static Scope<Mesh> Create(GraphicsContext& context, const MeshData& data);

        /// @brief Sends a mesh of vertices of any layout to the graphics card, for example with fewer attributes than
        /// Vertex.
        /// @param context The context that draws the mesh.
        /// @param vertices The bytes of the vertices, for example `std::as_bytes(std::span(vertices))`.
        /// @param layout How the attributes of a vertex are stored.
        /// @param indices The indices of the vertices of each triangle.
        /// @return The mesh, or nullptr if it is refused, as with the other Create().
        static Scope<Mesh> Create(GraphicsContext& context, std::span<const std::byte> vertices, const VertexLayout& layout,
                                  std::span<const std::uint32_t> indices);

        ~Mesh() = default;

        Mesh(const Mesh&) = delete;
        Mesh& operator=(const Mesh&) = delete;
        Mesh(Mesh&&) = delete;
        Mesh& operator=(Mesh&&) = delete;

        /// @brief Returns the vertex array of the mesh, with its vertex buffer and its index buffer.
        const VertexArray& GetVertexArray() const { return *m_VertexArray; }

    private:
        explicit Mesh(Scope<VertexArray> vertexArray);

        Scope<VertexArray> m_VertexArray;
    };
}
