#include <quasar/scene/FlyCameraController.h>

#include <glm/geometric.hpp>
#include <glm/gtc/quaternion.hpp>
#include <glm/trigonometric.hpp>

#include <algorithm>
#include <cmath>

namespace Quasar
{
    void FlyCameraController::Update(Timestep timestep, const FlyCameraInput& input, TransformComponent& transform)
    {
        SetRotation(m_Yaw - (input.Look.x * m_Settings.Sensitivity), m_Pitch - (input.Look.y * m_Settings.Sensitivity));
        transform.Rotation =
            glm::angleAxis(glm::radians(m_Yaw), glm::vec3(0.0f, 1.0f, 0.0f)) * glm::angleAxis(glm::radians(m_Pitch), glm::vec3(1.0f, 0.0f, 0.0f));

        const glm::vec3 direction =
            (transform.GetRight() * input.Move.x) + (glm::vec3(0.0f, 1.0f, 0.0f) * input.Move.y) + (transform.GetForward() * input.Move.z);
        const float speed = input.Fast ? m_Settings.FastSpeed : m_Settings.Speed;
        transform.Position += direction / std::max(glm::length(direction), 1.0f) * speed * timestep.GetSeconds();
    }

    void FlyCameraController::SetRotation(float yaw, float pitch)
    {
        m_Yaw = yaw;
        const float limit = std::min(std::abs(m_Settings.MaxPitch), 90.0f);
        m_Pitch = std::clamp(pitch, -limit, limit);
    }
}
