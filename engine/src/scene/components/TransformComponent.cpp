#include <quasar/scene/components/TransformComponent.h>

#include <glm/geometric.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/quaternion.hpp>
#include <glm/mat3x3.hpp>
#include <glm/matrix.hpp>
#include <glm/vec3.hpp>

namespace Quasar
{
    glm::mat4 TransformComponent::GetMatrix() const
    {
        return glm::translate(glm::mat4(1.0f), Position) * glm::mat4_cast(Rotation) * glm::scale(glm::mat4(1.0f), Scale);
    }

    void TransformComponent::SetMatrix(const glm::mat4& matrix)
    {
        const glm::mat3 basis(matrix);
        Position = glm::vec3(matrix[3]);
        Scale = {glm::length(basis[0]), glm::length(basis[1]), glm::length(basis[2])};
        if (glm::determinant(basis) < 0.0f)
        {
            Scale.x = -Scale.x;
        }
        if (Scale.x != 0.0f && Scale.y != 0.0f && Scale.z != 0.0f)
        {
            Rotation = glm::quat_cast(glm::mat3(basis[0] / Scale.x, basis[1] / Scale.y, basis[2] / Scale.z));
        }
    }

    glm::vec3 TransformComponent::GetEulerAngles() const
    {
        return glm::eulerAngles(Rotation);
    }

    void TransformComponent::SetEulerAngles(const glm::vec3& angles)
    {
        Rotation = glm::quat(angles);
    }

    glm::vec3 TransformComponent::GetForward() const
    {
        return Rotation * glm::vec3(0.0f, 0.0f, -1.0f);
    }

    glm::vec3 TransformComponent::GetRight() const
    {
        return Rotation * glm::vec3(1.0f, 0.0f, 0.0f);
    }

    glm::vec3 TransformComponent::GetUp() const
    {
        return Rotation * glm::vec3(0.0f, 1.0f, 0.0f);
    }
}
