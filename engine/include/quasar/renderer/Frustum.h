#pragma once

/// \file
/// \brief The volume that a camera sees, to skip the meshes outside of it.
/// \author Kurogenshi
/// \date 2026-11-26

#include <quasar/renderer/BoundingBox.h>

#include <glm/mat4x4.hpp>
#include <glm/vec4.hpp>

#include <array>

namespace Quasar
{
    /// @brief The volume that a camera sees, bounded by 6 planes: left, right, bottom, top, near and far.
    ///
    /// It is made from the view-projection matrix of a camera, whose depths go from 0 at the near plane to 1 at the
    /// far plane, as with Camera::GetViewProjection().
    class Frustum
    {
    public:
        /// @brief Makes the volume seen through a view-projection matrix.
        static Frustum FromViewProjection(const glm::mat4& viewProjection);

        /// @brief Returns false if a box is fully outside of one of the planes, so that nothing of it can be seen. A box
        /// close to a corner of the volume can still give true without being seen.
        bool Intersects(const BoundingBox& box) const;

    private:
        std::array<glm::vec4, 6> m_Planes{};
    };
}
