#pragma once

#include <unordered_map>
#include <memory>
#include <string>
#include <optional>
#include <QuasarEngine/Scene/Camera.h>
#include <QuasarEngine/ECS/Registry.h>
#include <QuasarEngine/Core/UUID.h>

namespace QuasarEngine
{
    class Entity;

    using EntityMap = std::unordered_map<UUID, entt::entity>;
    using NameMap = std::unordered_map<std::string, entt::entity>;

    class Scene
    {
    public:
        Scene();
        ~Scene();

        Entity CreateEntity(const std::string& name = std::string());
        Entity CreateEntityWithUUID(UUID uuid, const std::string& name = std::string());

        void DestroyEntity(UUID uuid);

        std::optional<Entity> GetEntityByUUID(UUID uuid) const;
		std::optional<Entity> GetEntityByName(const std::string& name) const;

        void Update(double deltaTime);
        void UpdateRuntime(double deltaTime);

        void OnRuntimeStart();
        void OnRuntimeStop();

        std::optional<Entity> GetPrimaryCameraEntity() const;
        Camera& GetPrimaryCamera();
        bool HasPrimaryCamera() const;

        bool isOnRuntime() const { return m_OnRuntime; }

        void ClearEntities();

        Registry* GetRegistry() { return m_Registry.get(); }

        bool IsEmpty() const { return m_EntityMap.empty(); }

        template<typename... Components>
        auto GetAllEntitiesWith()
        {
            return m_Registry->GetRegistry().view<Components...>();
        }

        template<typename... Owned, typename... Get>
        auto Group(entt::get_t<Get...> getter)
        {
            return m_Registry.group<Owned...>(getter);
        }

    private:
        EntityMap m_EntityMap;
        NameMap m_NameMap;

        std::unique_ptr<Registry> m_Registry;

        bool m_OnRuntime;
        UUID m_PrimaryCameraUUID;

        std::unordered_set<UUID> m_PendingEntityDestructions;

        void DestroyEntityNow(Entity entity);

        void ProcessEntityDestructions();

        void RegisterEntityName(const std::string& name, entt::entity entity);
        void UnregisterEntityName(const std::string& name);
        void UpdatePrimaryCameraCache();
    };
}