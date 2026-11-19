#pragma once

#include <quasar/scene/components/TransformComponent.h>
#include <quasar/time/Timestep.h>

#include <glm/vec2.hpp>
#include <glm/vec3.hpp>

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
        void Update(Timestep timestep, const FlyCameraInput& input, TransformComponent& transform);

        float GetYaw() const { return m_Yaw; }
        float GetPitch() const { return m_Pitch; }

    private:
        float m_Yaw = 0.0f;
        float m_Pitch = -10.0f;
    };
}
