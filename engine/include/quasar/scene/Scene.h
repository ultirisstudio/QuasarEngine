#pragma once

/// \file
/// \brief Scenes: the entities of a level, with their components.
/// \author Kurogenshi
/// \date 2026-11-19

#include <quasar/core/UUID.h>
#include <quasar/scene/Entity.h>

#include <entt/entity/registry.hpp>

#include <cstddef>
#include <cstdint>
#include <string>
#include <unordered_map>

namespace Quasar
{
    /// @brief A set of entities, for example a level.
    ///
    /// The entities are created and destroyed by the scene, which finds them by their UUID. Their components are stored
    /// in an EnTT registry: a system reads the entities that have some components with
    /// `GetRegistry().view<TransformComponent, MyComponent>()`, or more simply with ForEach().
    class Scene
    {
    public:
        Scene() = default;
        ~Scene() = default;

        Scene(const Scene&) = delete;
        Scene& operator=(const Scene&) = delete;
        Scene(Scene&&) = delete;
        Scene& operator=(Scene&&) = delete;

        /// @brief Creates an entity with a new UUID, a name and a TransformComponent at the origin, at the root of the
        /// scene.
        Entity CreateEntity(std::string name = "Entity");

        /// @brief Creates an entity with a given UUID, for example when a scene is loaded, a name and a
        /// TransformComponent at the origin, at the root of the scene.
        /// @return The entity, or an invalid entity if the UUID is not valid or is already used. The reason is logged
        /// as an engine error.
        Entity CreateEntity(UUID id, std::string name);

        /// @brief Destroys an entity, its components and all its descendants, and removes it from the children of its
        /// parent. The entity can be the current one of a ForEach() or of a view if it has no children, but not
        /// another one. An invalid entity is refused with an engine error.
        void DestroyEntity(Entity entity);

        /// @brief Returns the entity with this UUID, or an invalid entity.
        Entity FindEntity(UUID id);

        /// @brief Returns an entity whose CameraComponent is primary, or an invalid entity if there is none. With several
        /// primary cameras, any of them can be returned.
        Entity FindPrimaryCamera();

        /// @brief Gives a viewport size to the camera of every CameraComponent, for example the size of the window when
        /// it changes. See Camera::SetViewportSize().
        void SetViewportSize(std::uint32_t width, std::uint32_t height);

        /// @brief Returns the number of entities.
        std::size_t GetEntityCount() const { return m_Entities.size(); }

        /// @brief Calls a function for each entity that has all the components, with the entity and its components.
        ///
        /// For example `scene.ForEach<TransformComponent>([](Entity entity, TransformComponent& transform) { ... });`.
        template <typename... Components, typename Function>
        void ForEach(Function function);

        /// @brief Returns the EnTT registry that stores the components.
        entt::registry& GetRegistry() { return m_Registry; }

        /// @brief Returns the EnTT registry that stores the components.
        const entt::registry& GetRegistry() const { return m_Registry; }

    private:
        Entity Create(UUID id, std::string name);

        entt::registry m_Registry;
        std::unordered_map<UUID, entt::entity> m_Entities;
    };

    template <typename... Components, typename Function>
    void Scene::ForEach(Function function)
    {
        auto view = m_Registry.view<Components...>();
        for (const entt::entity handle : view)
        {
            function(Entity(handle, this), view.template get<Components>(handle)...);
        }
    }
}
