#pragma once

/// \file
/// \brief The place of an entity in the hierarchy of its scene.
/// \author Kurogenshi
/// \date 2026-11-20

#include <quasar/core/UUID.h>

#include <vector>

namespace Quasar
{
    /// @brief The parent and the children of an entity. The scene gives it to every entity and keeps both sides of each
    /// link in agreement, so it is only read: Entity::SetParent() changes it.
    ///
    /// The links are UUIDs, so they stay the same when the scene is saved and loaded.
    struct RelationshipComponent
    {
        UUID Parent{};                ///< The parent, or an invalid UUID for an entity at the root of the scene.
        std::vector<UUID> Children{}; ///< The children, in the order they were given their parent.
    };
}
