#include "qepch.h"

#include <QuasarEngine/Scene/Scene.h>
#include <QuasarEngine/Entity/Entity.h>
#include <QuasarEngine/Core/Application.h>
#include <QuasarEngine/Core/Window.h>
#include <QuasarEngine/Core/UUID.h>
#include <QuasarEngine/Entity/Components/TransformComponent.h>
#include <QuasarEngine/Entity/Components/CameraComponent.h>
#include <QuasarEngine/Entity/Components/HierarchyComponent.h>
#include <QuasarEngine/Entity/Components/TagComponent.h>
#include <QuasarEngine/Physic/PhysicEngine.h>
#include <QuasarEngine/Core/Input.h>

#include "QuasarEngine/Entity/Components/Physics/RigidBodyComponent.h"

namespace QuasarEngine
{
    Scene::Scene() :
        m_OnRuntime(false),
        m_LightsCount(0),
        m_PrimaryCameraUUID(0)
    {
        //m_Skybox = std::make_unique<Skybox>();
        m_Registry = std::make_unique<Registry>();
    }

    Scene::~Scene()
    {
        ClearEntities();
    }

    Entity Scene::CreateEntity(const std::string& name)
    {
        return CreateEntityWithUUID(UUID(), name);
    }

    Entity Scene::CreateEntityWithUUID(UUID uuid, const std::string& name)
    {
        Entity entity = { m_Registry->CreateEntity(), m_Registry.get() };
        entity.AddComponent<IDComponent>(uuid);
        entity.AddComponent<TransformComponent>();
        entity.AddComponent<HierarchyComponent>();
        auto& tag = entity.AddComponent<TagComponent>();
        tag.Tag = name.empty() ? "Entity" : name;

        m_EntityMap[uuid] = entity;
        RegisterEntityName(tag.Tag, entity);

        if (entity.HasComponent<CameraComponent>() && entity.GetComponent<CameraComponent>().Primary)
        {
            m_PrimaryCameraUUID = uuid;
        }

        return entity;
    }

    void Scene::DestroyEntity(Entity entity)
    {
        /*RenderCommand::WaitDevice();

        if (entity == Entity::Null() || !entity.IsValid())
            return;

        if (entity.HasComponent<HierarchyComponent>())
        {
            auto& hierarchy = entity.GetComponent<HierarchyComponent>();
            std::vector<UUID> childrenCopy = hierarchy.m_Childrens;
            for (auto childUUID : childrenCopy)
            {
                auto it = m_EntityMap.find(childUUID);
                if (it != m_EntityMap.end())
                {
                    Entity childEntity{ it->second, m_Registry.get() };
                    if (childEntity != entity && childEntity.IsValid())
                        DestroyEntity(childEntity);
                }
            }
        }

        if (entity.HasComponent<TagComponent>())
        {
            const std::string& tag = entity.GetComponent<TagComponent>().Tag;
            UnregisterEntityName(tag);
        }

        UUID uuid = entity.GetUUID();

        if (m_PrimaryCameraUUID == uuid)
            m_PrimaryCameraUUID = 0;

        m_EntityMap.erase(uuid);

        if (entity.IsValid())
            m_Registry->DestroyEntity(entity);*/

        if (entity == Entity::Null() || !entity.IsValid())
            return;

        UUID uuid = entity.GetUUID();
        m_PendingEntityDestructions.insert(uuid);
    }

    void Scene::ProcessEntityDestructions()
    {
        if (m_PendingEntityDestructions.empty())
            return;

        std::vector<UUID> toDelete(m_PendingEntityDestructions.begin(), m_PendingEntityDestructions.end());
        m_PendingEntityDestructions.clear();

        for (UUID uuid : toDelete)
        {
            auto it = m_EntityMap.find(uuid);
            if (it != m_EntityMap.end())
            {
                Entity entity{ it->second, m_Registry.get() };
                DestroyEntityNow(entity);
            }
        }
    }

    void Scene::DestroyEntityNow(Entity entity)
    {
        if (entity == Entity::Null() || !entity.IsValid())
            return;

        if (entity.HasComponent<HierarchyComponent>())
        {
            auto& hierarchy = entity.GetComponent<HierarchyComponent>();
            std::vector<UUID> childrenCopy = hierarchy.m_Childrens;
            for (auto childUUID : childrenCopy)
            {
                auto it = m_EntityMap.find(childUUID);
                if (it != m_EntityMap.end())
                {
                    Entity childEntity{ it->second, m_Registry.get() };
                    if (childEntity != entity && childEntity.IsValid())
                        DestroyEntityNow(childEntity);
                }
            }
        }

        if (entity.HasComponent<TagComponent>())
        {
            const std::string& tag = entity.GetComponent<TagComponent>().Tag;
            UnregisterEntityName(tag);
        }

        UUID uuid = entity.GetUUID();

        if (m_PrimaryCameraUUID == uuid)
            m_PrimaryCameraUUID = 0;

        m_EntityMap.erase(uuid);

        if (entity.IsValid())
            m_Registry->DestroyEntity(entity);
    }

    std::optional<Entity> Scene::GetEntityByUUID(UUID uuid) const
    {
        auto it = m_EntityMap.find(uuid);
        if (it != m_EntityMap.end())
        {
            assert(m_Registry && "Registry is nullptr");
            assert(m_Registry->GetRegistry().valid(it->second) && "Entity is invalid");
            return Entity{ it->second, m_Registry.get() };
        }
        return std::nullopt;
    }

    std::optional<Entity> Scene::GetEntityByName(const std::string& name) const
    {
        auto it = m_NameMap.find(name);
        if (it != m_NameMap.end())
        {
            assert(m_Registry && "Registry is nullptr");
            assert(m_Registry->GetRegistry().valid(it->second) && "Entity is invalid");
            return Entity{ it->second, m_Registry.get() };
        }
        return std::nullopt;
    }

    void Scene::Update(double deltaTime)
    {
        ProcessEntityDestructions();

        if (m_OnRuntime)
            UpdateRuntime(deltaTime);
    }

    void Scene::UpdateRuntime(double deltaTime)
    {
        PhysicEngine::Instance().Step(deltaTime);

        for (auto e : GetAllEntitiesWith<RigidBodyComponent>())
        {
            Entity entity = { e, m_Registry.get() };
            entity.GetComponent<RigidBodyComponent>().Update(deltaTime);
        }

        Renderer::m_SceneData.m_ScriptSystem->Update(deltaTime);

        if (Input::IsKeyJustPressed(Key::Escape))
        {
            Application::Get().GetWindow().SetInputMode(false, false);
		}
    }

    void Scene::OnRuntimeStart()
    {
        m_OnRuntime = true;

        UpdatePrimaryCameraCache();

        Renderer::m_SceneData.m_ScriptSystem->Start();

        Application::Get().GetWindow().SetInputMode(true, true);
    }

    void Scene::OnRuntimeStop()
    {
        m_OnRuntime = false;

        Renderer::m_SceneData.m_ScriptSystem->Stop();
    }

    void Scene::ClearEntities()
    {
        std::vector<uint64_t> uuids;
        uuids.reserve(m_EntityMap.size());
        for (const auto& [uuid, handle] : m_EntityMap)
            uuids.push_back(uuid);

        for (uint64_t uuid : uuids)
        {
            DestroyEntity(Entity{ m_EntityMap[uuid], m_Registry.get() });
        }

        //m_EntityMap.clear();
        //m_NameMap.clear();
        //m_Registry->ClearRegistry();
        m_PrimaryCameraUUID = 0;
    }

    std::optional<Entity> Scene::GetPrimaryCameraEntity() const
    {
        if (m_PrimaryCameraUUID)
        {
            auto it = m_EntityMap.find(m_PrimaryCameraUUID);
            if (it != m_EntityMap.end())
                return Entity{ it->second, m_Registry.get() };
        }
        
        auto view = m_Registry->GetRegistry().view<CameraComponent>();
        for (auto entity : view)
        {
            auto& cameraComponent = view.get<CameraComponent>(entity);
            if (cameraComponent.Primary)
                return Entity{ entity, m_Registry.get() };
        }
        return std::nullopt;
    }

    Camera& Scene::GetPrimaryCamera()
    {
        auto entOpt = GetPrimaryCameraEntity();
        if (!entOpt)
            throw std::runtime_error("No primary camera found in scene.");
        return entOpt.value().GetComponent<CameraComponent>().GetCamera();
    }

    bool Scene::HasPrimaryCamera() const
    {
        return GetPrimaryCameraEntity().has_value();
    }

    void Scene::RegisterEntityName(const std::string& name, entt::entity entity)
    {
        if (!name.empty())
            m_NameMap[name] = entity;
    }

    void Scene::UnregisterEntityName(const std::string& name)
    {
        if (!name.empty())
            m_NameMap.erase(name);
    }

    void Scene::UpdatePrimaryCameraCache()
    {
        m_PrimaryCameraUUID = 0;
        auto view = m_Registry->GetRegistry().view<IDComponent, CameraComponent>();
        for (auto entity : view)
        {
            auto& cameraComponent = view.get<CameraComponent>(entity);
            if (cameraComponent.Primary)
            {
                auto& id = view.get<IDComponent>(entity);
                m_PrimaryCameraUUID = id.ID;
                break;
            }
        }
    }
}
