#include "FlyCamera.h"

#include <glm/geometric.hpp>
#include <glm/gtc/quaternion.hpp>
#include <glm/trigonometric.hpp>

#include <algorithm>

namespace Quasar::Sandbox
{
    namespace
    {
        constexpr float Speed = 5.0f;
        constexpr float FastSpeed = 15.0f;
        constexpr float Sensitivity = 0.1f;
        constexpr float MaxPitch = 89.0f;
    }

    void FlyCamera::Update(Timestep timestep, const FlyCameraInput& input, TransformComponent& transform)
    {
        m_Yaw -= input.Look.x * Sensitivity;
        m_Pitch = std::clamp(m_Pitch - (input.Look.y * Sensitivity), -MaxPitch, MaxPitch);
        transform.Rotation =
            glm::angleAxis(glm::radians(m_Yaw), glm::vec3(0.0f, 1.0f, 0.0f)) * glm::angleAxis(glm::radians(m_Pitch), glm::vec3(1.0f, 0.0f, 0.0f));

        const glm::vec3 right = transform.Rotation * glm::vec3(1.0f, 0.0f, 0.0f);
        const glm::vec3 forward = transform.Rotation * glm::vec3(0.0f, 0.0f, -1.0f);
        const glm::vec3 direction = (right * input.Move.x) + (glm::vec3(0.0f, 1.0f, 0.0f) * input.Move.y) + (forward * input.Move.z);
        if (glm::dot(direction, direction) > 0.0f)
        {
            const float speed = input.Fast ? FastSpeed : Speed;
            transform.Position += glm::normalize(direction) * speed * timestep.GetSeconds();
        }
    }
}
