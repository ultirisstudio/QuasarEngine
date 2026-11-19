#pragma once

/// \file
/// \brief The name of an entity.
/// \author Kurogenshi
/// \date 2026-11-19

#include <string>

namespace Quasar
{
    /// @brief The name of an entity, shown in the editor. The scene gives it to every entity, and names can repeat.
    struct NameComponent
    {
        std::string Name; ///< The name.
    };
}
