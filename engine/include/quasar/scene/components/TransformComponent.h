#pragma once

/// \file
/// \brief The position, the rotation and the scale of an entity.
/// \author Kurogenshi
/// \date 2026-11-19

#include <glm/gtc/quaternion.hpp>
#include <glm/mat4x4.hpp>
#include <glm/vec3.hpp>

namespace Quasar
{
    /// @brief The position, the rotation and the scale of an entity.
    struct TransformComponent
    {
        glm::vec3 Position{0.0f};                   ///< The position.
        glm::quat Rotation{1.0f, 0.0f, 0.0f, 0.0f}; ///< The rotation, as a unit quaternion.
        glm::vec3 Scale{1.0f};                      ///< The scale along each axis.

        /// @brief Returns the matrix that scales, then rotates, then moves the entity.
        glm::mat4 GetMatrix() const;

        /// @brief Returns the rotation as angles around X, Y and Z, in radians, for example to edit it. Near 90 degrees
        /// around Y, several angles give the same rotation, and any of them can be returned.
        glm::vec3 GetEulerAngles() const;

        /// @brief Sets the rotation from angles around X, Y and Z, in radians.
        void SetEulerAngles(const glm::vec3& angles);
    };
}
