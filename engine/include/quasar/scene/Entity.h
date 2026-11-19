#pragma once

/// \file
/// \brief Entities: the objects of a scene, made of components.
/// \author Kurogenshi
/// \date 2026-11-19

#include <quasar/core/UUID.h>
#include <quasar/log/Log.h>
#include <quasar/scene/components/IdComponent.h>
#include <quasar/scene/components/NameComponent.h>
#include <quasar/scene/components/RelationshipComponent.h>
#include <quasar/scene/components/TransformComponent.h>

#include <entt/entity/registry.hpp>
#include <glm/mat4x4.hpp>

#include <string>
#include <type_traits>
#include <utility>
#include <vector>

namespace Quasar
{
    class Scene;

    /// @brief A handle to an object of a scene, whose data are its components.
    ///
    /// An entity is a small value that can be copied freely. It stays valid until the entity is destroyed or its scene
    /// is destroyed. Every entity has an IdComponent, a NameComponent and a RelationshipComponent, which the scene
    /// manages.
    ///
    /// Entities form a hierarchy: each one has at most one parent and any number of children, set with SetParent().
    /// The TransformComponent of an entity is relative to its parent, so moving a parent moves its children.
    class Entity
    {
    public:
        /// @brief Creates an invalid entity.
        Entity() = default;

        /// @brief Creates the handle of an entity of a scene, for example one found in a view of Scene::GetRegistry().
        Entity(entt::entity handle, Scene* scene) : m_Handle(handle), m_Scene(scene) {}

        /// @brief Returns true if the entity exists in its scene.
        bool IsValid() const;

        /// @brief Returns IsValid().
        explicit operator bool() const { return IsValid(); }

        /// @brief Adds a component to the entity, which must be valid.
        /// @return The new component. If the entity already has one, it is kept, returned and an engine error is logged.
        template <typename T, typename... Args>
        T& Add(Args&&... args);

        /// @brief Returns a component of the entity, which must be valid and have it.
        template <typename T>
        T& Get() const;

        /// @brief Returns a component of the entity, or nullptr if the entity does not have it or is not valid.
        template <typename T>
        T* TryGet() const;

        /// @brief Returns true if the entity is valid and has the component.
        template <typename T>
        bool Has() const;

        /// @brief Removes a component of the entity, which must be valid.
        /// @return True if the entity had the component.
        template <typename T>
        bool Remove();

        /// @brief Returns the permanent identifier of the entity, which must be valid.
        UUID GetUUID() const { return Get<IdComponent>().Id; }

        /// @brief Returns the name of the entity, which must be valid.
        const std::string& GetName() const { return Get<NameComponent>().Name; }

        /// @brief Moves the entity under a parent, after its other children, or to the root of the scene.
        /// @param parent An entity of the same scene, or an invalid entity for the root.
        /// @param keepWorldTransform Keeps the entity at the same place in the world by changing its TransformComponent.
        /// Otherwise its TransformComponent is kept, and becomes relative to the new parent.
        /// @return False if the entity is not valid, or if the parent is in another scene, is the entity itself or one
        /// of its descendants. The reason is logged as an engine error, and nothing changes.
        bool SetParent(Entity parent, bool keepWorldTransform = true);

        /// @brief Returns the parent of the entity, which must be valid, or an invalid entity at the root of the scene.
        Entity GetParent() const;

        /// @brief Returns the children of the entity, which must be valid, in the order they were given their parent.
        std::vector<Entity> GetChildren() const;

        /// @brief Returns true if the entity is a child of the ancestor, or a child of one of its descendants.
        bool IsDescendantOf(Entity ancestor) const;

        /// @brief Returns the matrix that places the entity, which must be valid, in the world: the one of its
        /// TransformComponent, after the ones of its ancestors. An entity without a TransformComponent does not move.
        glm::mat4 GetWorldMatrix() const;

        /// @brief Returns the position, the rotation and the scale of the entity in the world, from GetWorldMatrix().
        /// See TransformComponent::SetMatrix().
        TransformComponent GetWorldTransform() const;

        /// @brief Returns the EnTT handle of the entity.
        entt::entity GetHandle() const { return m_Handle; }

        /// @brief Returns the scene of the entity, or nullptr for a default entity.
        Scene* GetScene() const { return m_Scene; }

        /// @brief Two entities are equal if they are the same entity of the same scene.
        bool operator==(const Entity&) const = default;

    private:
        entt::registry& GetRegistry() const;

        entt::entity m_Handle = entt::null;
        Scene* m_Scene = nullptr;
    };

    template <typename T, typename... Args>
    T& Entity::Add(Args&&... args)
    {
        static_assert(!std::is_same_v<T, IdComponent> && !std::is_same_v<T, NameComponent> && !std::is_same_v<T, RelationshipComponent>,
                      "The scene manages the id, the name and the relationship of an entity");
        if (T* existing = TryGet<T>(); existing != nullptr)
        {
            QUASAR_ENGINE_ERROR("The entity '{}' already has this component", GetName());
            return *existing;
        }
        return GetRegistry().template emplace<T>(m_Handle, std::forward<Args>(args)...);
    }

    template <typename T>
    T& Entity::Get() const
    {
        return GetRegistry().template get<T>(m_Handle);
    }

    template <typename T>
    T* Entity::TryGet() const
    {
        return IsValid() ? GetRegistry().template try_get<T>(m_Handle) : nullptr;
    }

    template <typename T>
    bool Entity::Has() const
    {
        return IsValid() && GetRegistry().template all_of<T>(m_Handle);
    }

    template <typename T>
    bool Entity::Remove()
    {
        static_assert(!std::is_same_v<T, IdComponent> && !std::is_same_v<T, NameComponent> && !std::is_same_v<T, RelationshipComponent>,
                      "The scene manages the id, the name and the relationship of an entity");
        return GetRegistry().template remove<T>(m_Handle) > 0;
    }
}
