#include "FlyCamera.h"

#include <glm/geometric.hpp>
#include <glm/gtc/quaternion.hpp>
#include <glm/trigonometric.hpp>

#include <algorithm>
#include <cstdint>

namespace Quasar::Sandbox
{
    namespace
    {
        constexpr float Speed = 5.0f;
        constexpr float FastSpeed = 15.0f;
        constexpr float Sensitivity = 0.1f;
        constexpr float MaxPitch = 89.0f;
        constexpr float StartPitch = -10.0f;
    }

    FlyCamera::FlyCamera() : m_Pitch(StartPitch)
    {
        m_Camera.SetPerspective({.VerticalFov = glm::radians(60.0f), .Near = 0.1f, .Far = 500.0f});
        m_Camera.SetPosition({0.0f, 2.0f, 8.0f});
        UpdateOrientation();
    }

    void FlyCamera::Update(Timestep timestep, const FlyCameraInput& input)
    {
        if (input.Look.x != 0.0f || input.Look.y != 0.0f)
        {
            m_Yaw -= input.Look.x * Sensitivity;
            m_Pitch = std::clamp(m_Pitch - (input.Look.y * Sensitivity), -MaxPitch, MaxPitch);
            UpdateOrientation();
        }

        const glm::vec3 direction =
            (m_Camera.GetRight() * input.Move.x) + (glm::vec3(0.0f, 1.0f, 0.0f) * input.Move.y) + (m_Camera.GetForward() * input.Move.z);
        if (glm::dot(direction, direction) > 0.0f)
        {
            const float speed = input.Fast ? FastSpeed : Speed;
            m_Camera.SetPosition(m_Camera.GetPosition() + (glm::normalize(direction) * speed * timestep.GetSeconds()));
        }
    }

    void FlyCamera::SetViewportSize(std::uint32_t width, std::uint32_t height)
    {
        m_Camera.SetViewportSize(width, height);
    }

    void FlyCamera::UpdateOrientation()
    {
        const glm::quat yaw = glm::angleAxis(glm::radians(m_Yaw), glm::vec3(0.0f, 1.0f, 0.0f));
        const glm::quat pitch = glm::angleAxis(glm::radians(m_Pitch), glm::vec3(1.0f, 0.0f, 0.0f));
        m_Camera.SetOrientation(yaw * pitch);
    }
}
