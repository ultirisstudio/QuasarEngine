#pragma once

/// \file
/// \brief A controller that flies a camera, or any entity, around a scene.
/// \author Kurogenshi
/// \date 2026-11-19

#include <quasar/scene/components/TransformComponent.h>
#include <quasar/time/Timestep.h>

#include <glm/vec2.hpp>
#include <glm/vec3.hpp>

namespace Quasar
{
    /// @brief What the player asks the FlyCameraController for during one frame.
    struct FlyCameraInput
    {
        /// The direction of the move, each axis between -1 and 1: x to the right, y up in the world, z forward. For
        /// example `Input::GetKeyAxis(Key::A, Key::D)` for x. A move shorter than 1, from a gamepad stick, is slower,
        /// and a longer one goes at full speed.
        glm::vec3 Move{0.0f};

        /// How far the view turns, in pixels of mouse movement: x to the right, y down, as Input::GetMouseDelta().
        glm::vec2 Look{0.0f};

        bool Fast = false; ///< Moves at FlyCameraSettings::FastSpeed.
    };

    /// @brief Settings of a FlyCameraController.
    struct FlyCameraSettings
    {
        float Speed = 5.0f;       ///< The speed of the moves, in units per second.
        float FastSpeed = 15.0f;  ///< The speed of the fast moves, in units per second.
        float Sensitivity = 0.1f; ///< The degrees that the view turns for each pixel of FlyCameraInput::Look.
        float MaxPitch = 89.0f;   ///< The most that the view can look up or down, in degrees, up to 90.
    };

    /// @brief Flies a transform around a scene, as in an editor or a game in free camera.
    ///
    /// The view turns around the world Y axis by its yaw, then around its own X axis by its pitch, and never rolls. The
    /// moves follow the direction of the view, except up and down, which follow the world Y axis. The scale of the
    /// transform is kept.
    class FlyCameraController
    {
    public:
        /// @brief Creates a controller that looks toward -Z.
        explicit FlyCameraController(const FlyCameraSettings& settings = {}) : m_Settings(settings) {}

        /// @brief Turns and moves a transform for one frame.
        void Update(Timestep timestep, const FlyCameraInput& input, TransformComponent& transform);

        /// @brief Sets the direction of the view, in degrees. The pitch is clamped to FlyCameraSettings::MaxPitch. The
        /// transform changes at the next Update().
        void SetRotation(float yaw, float pitch);

        float GetYaw() const { return m_Yaw; }     ///< Returns the turn around the world Y axis, in degrees.
        float GetPitch() const { return m_Pitch; } ///< Returns the turn up or down, in degrees.

        /// @brief Returns the settings of the controller.
        const FlyCameraSettings& GetSettings() const { return m_Settings; }

        /// @brief Replaces the settings of the controller.
        void SetSettings(const FlyCameraSettings& settings) { m_Settings = settings; }

    private:
        FlyCameraSettings m_Settings;
        float m_Yaw = 0.0f;
        float m_Pitch = 0.0f;
    };
}
