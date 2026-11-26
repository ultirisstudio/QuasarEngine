#pragma once

/// \file
/// \brief The box aligned with the axes that holds a mesh, used to skip the meshes that cannot be seen.
/// \author Kurogenshi
/// \date 2026-11-26

#include <glm/mat4x4.hpp>
#include <glm/vec3.hpp>

#include <span>

namespace Quasar
{
    /// @brief A box whose faces are aligned with the axes, given by its smallest and its largest corners.
    struct BoundingBox
    {
        glm::vec3 Min{0.0f}; ///< The corner with the smallest coordinates.
        glm::vec3 Max{0.0f}; ///< The corner with the largest coordinates.

        /// @brief Returns the smallest box that holds points, or a box of size 0 at the origin without points.
        static BoundingBox FromPoints(std::span<const glm::vec3> points);

        /// @brief Returns the smallest box aligned with the axes that holds this box once moved by a matrix, for
        /// example to place the box of a mesh in the world.
        BoundingBox Transformed(const glm::mat4& transform) const;

        /// @brief Compares the corners.
        bool operator==(const BoundingBox&) const = default;
    };
}
