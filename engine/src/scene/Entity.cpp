#include <quasar/scene/Entity.h>

#include <quasar/log/Log.h>
#include <quasar/scene/Scene.h>

#include <entt/entity/registry.hpp>
#include <glm/matrix.hpp>

#include <algorithm>
#include <vector>

namespace Quasar
{
    bool Entity::IsValid() const
    {
        return m_Scene != nullptr && m_Scene->GetRegistry().valid(m_Handle);
    }

    bool Entity::SetParent(Entity parent, bool keepWorldTransform)
    {
        if (!IsValid())
        {
            QUASAR_ENGINE_ERROR("Cannot give a parent to an entity that is not valid");
            return false;
        }
        if (parent.IsValid() && parent.GetScene() != m_Scene)
        {
            QUASAR_ENGINE_ERROR("Cannot give the entity '{}' a parent from another scene", GetName());
            return false;
        }
        if (parent == *this || parent.IsDescendantOf(*this))
        {
            QUASAR_ENGINE_ERROR("Cannot give the entity '{}' the parent '{}': it is the entity or one of its descendants", GetName(), parent.GetName());
            return false;
        }

        const Entity current = GetParent();
        if (parent.IsValid() ? parent == current : !current.IsValid())
        {
            return true;
        }

        const glm::mat4 world = GetWorldMatrix();
        const UUID id = GetUUID();
        if (current.IsValid())
        {
            std::erase(current.Get<RelationshipComponent>().Children, id);
        }
        Get<RelationshipComponent>().Parent = parent.IsValid() ? parent.GetUUID() : UUID();
        if (parent.IsValid())
        {
            parent.Get<RelationshipComponent>().Children.push_back(id);
        }

        if (auto* transform = TryGet<TransformComponent>(); keepWorldTransform && transform != nullptr)
        {
            transform->SetMatrix(parent.IsValid() ? glm::inverse(parent.GetWorldMatrix()) * world : world);
        }
        return true;
    }

    Entity Entity::GetParent() const
    {
        return m_Scene->FindEntity(Get<RelationshipComponent>().Parent);
    }

    std::vector<Entity> Entity::GetChildren() const
    {
        std::vector<Entity> children;
        for (const UUID child : Get<RelationshipComponent>().Children)
        {
            children.push_back(m_Scene->FindEntity(child));
        }
        return children;
    }

    bool Entity::IsDescendantOf(Entity ancestor) const
    {
        if (!IsValid())
        {
            return false;
        }
        for (Entity parent = GetParent(); parent.IsValid(); parent = parent.GetParent())
        {
            if (parent == ancestor)
            {
                return true;
            }
        }
        return false;
    }

    glm::mat4 Entity::GetWorldMatrix() const
    {
        glm::mat4 world(1.0f);
        for (Entity entity = *this; entity.IsValid(); entity = entity.GetParent())
        {
            if (const auto* transform = entity.TryGet<TransformComponent>(); transform != nullptr)
            {
                world = transform->GetMatrix() * world;
            }
        }
        return world;
    }

    TransformComponent Entity::GetWorldTransform() const
    {
        TransformComponent world;
        world.SetMatrix(GetWorldMatrix());
        return world;
    }

    entt::registry& Entity::GetRegistry() const
    {
        return m_Scene->GetRegistry();
    }
}
