#pragma once

/// \file
/// \brief The mesh and the material that draw an entity.
/// \author Kurogenshi
/// \date 2026-11-19

#include <quasar/renderer/Material.h>
#include <quasar/renderer/Mesh.h>

namespace Quasar
{
    /// @brief Draws an entity with a mesh and a material, at its place in the world. See Entity::GetWorldMatrix().
    ///
    /// The component does not own the mesh and the material, which must stay alive while the scene uses them. An
    /// entity without a mesh or a material is not drawn.
    struct MeshRendererComponent
    {
        const Quasar::Mesh* Mesh = nullptr;   ///< The mesh to draw.
        Quasar::Material* Material = nullptr; ///< The material to draw it with.
    };
}
