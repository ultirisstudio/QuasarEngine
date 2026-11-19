#include <quasar/scene/Entity.h>

#include <quasar/scene/Scene.h>

#include <entt/entity/registry.hpp>

namespace Quasar
{
    bool Entity::IsValid() const
    {
        return m_Scene != nullptr && m_Scene->GetRegistry().valid(m_Handle);
    }

    entt::registry& Entity::GetRegistry() const
    {
        return m_Scene->GetRegistry();
    }
}
