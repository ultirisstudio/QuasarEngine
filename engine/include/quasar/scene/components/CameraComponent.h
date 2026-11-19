#pragma once

/// \file
/// \brief A camera of a scene, which looks from the TransformComponent of its entity.
/// \author Kurogenshi
/// \date 2026-11-19

#include <quasar/renderer/Camera.h>

namespace Quasar
{
    /// @brief A camera of a scene, for example the one of the player.
    ///
    /// The camera looks from the position and with the rotation of the TransformComponent of its entity, without its
    /// scale, so the position and the orientation of Camera are not used. Its projection and its aspect ratio are the
    /// ones of Camera.
    struct CameraComponent
    {
        Quasar::Camera Camera; ///< The projection and the aspect ratio of the camera.
        bool Primary = true;   ///< The scene is drawn from its primary camera. See Scene::FindPrimaryCamera().
    };
}
