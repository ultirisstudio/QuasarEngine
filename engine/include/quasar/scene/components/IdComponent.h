#pragma once

/// \file
/// \brief The permanent identifier of an entity.
/// \author Kurogenshi
/// \date 2026-11-19

#include <quasar/core/UUID.h>

namespace Quasar
{
    /// @brief The permanent identifier of an entity, saved with the scene. The scene gives it to every entity.
    struct IdComponent
    {
        UUID Id; ///< The identifier, unique in the scene.
    };
}
