#include <quasar/scene/Scene.h>

#include <quasar/scene/components/CameraComponent.h>
#include <quasar/scene/components/IdComponent.h>
#include <quasar/scene/components/NameComponent.h>
#include <quasar/scene/components/TransformComponent.h>

#include <quasar/log/Log.h>

#include <string>
#include <utility>

namespace Quasar
{
    Entity Scene::CreateEntity(std::string name)
    {
        return Create(UUID::Generate(), std::move(name));
    }

    Entity Scene::CreateEntity(UUID id, std::string name)
    {
        if (!id.IsValid())
        {
            QUASAR_ENGINE_ERROR("Cannot create the entity '{}' with an invalid UUID", name);
            return {};
        }
        if (m_Entities.contains(id))
        {
            QUASAR_ENGINE_ERROR("Cannot create the entity '{}': the UUID {} is already used", name, id);
            return {};
        }
        return Create(id, std::move(name));
    }

    void Scene::DestroyEntity(Entity entity)
    {
        if (entity.GetScene() != this || !entity.IsValid())
        {
            QUASAR_ENGINE_ERROR("Cannot destroy an entity that is not in the scene");
            return;
        }
        m_Entities.erase(entity.GetUUID());
        m_Registry.destroy(entity.GetHandle());
    }

    Entity Scene::FindEntity(UUID id)
    {
        const auto found = m_Entities.find(id);
        return found != m_Entities.end() ? Entity(found->second, this) : Entity();
    }

    Entity Scene::FindPrimaryCamera()
    {
        for (const auto [handle, camera] : m_Registry.view<CameraComponent>().each())
        {
            if (camera.Primary)
            {
                return {handle, this};
            }
        }
        return {};
    }

    void Scene::SetViewportSize(std::uint32_t width, std::uint32_t height)
    {
        for (const auto [handle, camera] : m_Registry.view<CameraComponent>().each())
        {
            camera.Camera.SetViewportSize(width, height);
        }
    }

    Entity Scene::Create(UUID id, std::string name)
    {
        const entt::entity handle = m_Registry.create();
        m_Registry.emplace<IdComponent>(handle, id);
        m_Registry.emplace<NameComponent>(handle, std::move(name));
        m_Registry.emplace<TransformComponent>(handle);
        m_Entities.emplace(id, handle);
        return {handle, this};
    }
}
