#pragma once

/// \file
/// \brief Primitives: the mesh data of simple shapes, centered on the origin.
/// \author Kurogenshi
/// \date 2026-11-18

#include <quasar/renderer/Mesh.h>

#include <cstdint>

/// @brief Functions that build the mesh data of simple shapes.
///
/// The shapes are centered on the origin, with +Y up. Their triangles face outward, and the images of their textures
/// are upright when seen from outside: the faces whose normal is horizontal have v = 0 at their top. Their vertices have
/// tangents, for the normal maps. A size that is not a finite number above 0 gives empty data and logs an engine error.
namespace Quasar::Primitives
{
    /// @brief Builds a cube with 4 vertices per face, so that each face has its own normal and the whole image.
    /// @param size The length of the edges. The top face has v = 0 on its side toward -Z, and the bottom face on its
    /// side toward +Z.
    MeshData CreateCube(float size = 1.0f);

    /// @brief Builds a square in the XY plane, facing +Z.
    /// @param size The length of the edges.
    MeshData CreateQuad(float size = 1.0f);

    /// @brief Builds a UV sphere: rings of vertices from the top pole to the bottom one, like the lines of latitude.
    /// @param radius The radius.
    /// @param segments The number of vertices around each ring, at least 3.
    /// @param rings The number of bands from one pole to the other, at least 2.
    ///
    /// The texture wraps once around the sphere, with v = 0 at the top pole and u growing toward the east, so that an
    /// image of the Earth is upright and not mirrored. Fewer segments or rings give empty data and log an engine error.
    MeshData CreateSphere(float radius = 0.5f, std::uint32_t segments = 32, std::uint32_t rings = 16);
}
