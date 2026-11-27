#pragma once

/// \file
/// \brief Meshes: the triangles of an object, stored on the graphics card and ready to draw.
/// \author Kurogenshi
/// \date 2026-11-18

#include <quasar/core/Base.h>
#include <quasar/renderer/BoundingBox.h>
#include <quasar/renderer/GraphicsContext.h>
#include <quasar/renderer/VertexArray.h>
#include <quasar/renderer/VertexLayout.h>

#include <glm/vec2.hpp>
#include <glm/vec3.hpp>
#include <glm/vec4.hpp>

#include <cstddef>
#include <cstdint>
#include <optional>
#include <span>
#include <vector>

namespace Quasar
{
    /// @brief The standard vertex of the meshes, read by a vertex shader with `layout(location = 0) in vec3
    /// a_Position;`, `layout(location = 1) in vec3 a_Normal;`, `layout(location = 2) in vec2 a_TexCoord;` and
    /// `layout(location = 3) in vec4 a_Tangent;`.
    struct Vertex
    {
        glm::vec3 Position{0.0f}; ///< The position, in the space of the mesh.
        glm::vec3 Normal{0.0f};   ///< The direction the surface faces, of length 1.
        glm::vec2 TexCoord{0.0f}; ///< The texture coordinates, with v = 0 at the top of the image.

        /// The direction in which u grows, of length 1 and perpendicular to the normal, and in w the sign that gives
        /// the direction toward the top of the image: `cross(Normal, Tangent.xyz) * Tangent.w`. The normal maps use it.
        /// See MeshData::GenerateTangents().
        glm::vec4 Tangent{0.0f};

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

        /// @brief Computes the Tangent of each vertex from the positions and the texture coordinates of its
        /// triangles, averaged over the triangles that share the vertex. The Primitives already have them.
        ///
        /// A vertex whose triangles have no texture coordinates, or indices that are not vertices, gets a tangent
        /// perpendicular to its normal.
        void GenerateTangents();
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

        /// @brief Returns the box that holds the vertices, in the space of the mesh. It is computed from the attribute
        /// of location 0 when it is a `vec3`, as the position of Vertex, and is nullopt for the other layouts.
        const std::optional<BoundingBox>& GetBounds() const { return m_Bounds; }

    private:
        Mesh(Scope<VertexArray> vertexArray, const std::optional<BoundingBox>& bounds);

        Scope<VertexArray> m_VertexArray;
        std::optional<BoundingBox> m_Bounds;
    };
}
