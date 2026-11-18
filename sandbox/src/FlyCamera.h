#pragma once

#include <quasar/renderer/Camera.h>
#include <quasar/time/Timestep.h>

#include <glm/vec2.hpp>
#include <glm/vec3.hpp>

#include <cstdint>

namespace Quasar::Sandbox
{
    struct FlyCameraInput
    {
        glm::vec3 Move{0.0f};
        glm::vec2 Look{0.0f};
        bool Fast = false;
    };

    class FlyCamera
    {
    public:
        FlyCamera();

        void Update(Timestep timestep, const FlyCameraInput& input);
        void SetViewportSize(std::uint32_t width, std::uint32_t height);

        const Camera& GetCamera() const { return m_Camera; }
        float GetYaw() const { return m_Yaw; }
        float GetPitch() const { return m_Pitch; }

    private:
        void UpdateOrientation();

        Camera m_Camera;
        float m_Yaw = 0.0f;
        float m_Pitch = 0.0f;
    };
}
