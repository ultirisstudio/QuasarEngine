#include <quasar/scene/components/TransformComponent.h>

#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/quaternion.hpp>

namespace Quasar
{
    glm::mat4 TransformComponent::GetMatrix() const
    {
        return glm::translate(glm::mat4(1.0f), Position) * glm::mat4_cast(Rotation) * glm::scale(glm::mat4(1.0f), Scale);
    }

    glm::vec3 TransformComponent::GetEulerAngles() const
    {
        return glm::eulerAngles(Rotation);
    }

    void TransformComponent::SetEulerAngles(const glm::vec3& angles)
    {
        Rotation = glm::quat(angles);
    }
}
