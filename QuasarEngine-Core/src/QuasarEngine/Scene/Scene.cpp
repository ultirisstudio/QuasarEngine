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
#include <QuasarEngine/Entity/Components/Animation/AnimationComponent.h>
#include <QuasarEngine/Entity/Components/Particles/ParticleComponent.h>

namespace QuasarEngine
{
    Scene::Scene() :
        m_OnRuntime(false),
        m_PrimaryCameraUUID(0)
    {
        m_Registry = std::make_unique<Registry>();
    }

    Scene::~Scene()
    {
        ClearEntities();
        ProcessEntityDestructions();
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

    void Scene::DestroyEntity(UUID uuid)
    {
        if (uuid == UUID::Null()) return;
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
            if (it == m_EntityMap.end()) continue;

            std::vector<UUID> stack;
            std::vector<UUID> order;
            stack.push_back(uuid);

            while (!stack.empty())
            {
                UUID cur = stack.back(); stack.pop_back();
                auto itc = m_EntityMap.find(cur);
                if (itc == m_EntityMap.end()) continue;

                Entity e{ itc->second, m_Registry.get() };
                if (!e.IsValid()) { m_EntityMap.erase(cur); continue; }

                order.push_back(cur);

                if (e.HasComponent<HierarchyComponent>())
                {
                    auto& h = e.GetComponent<HierarchyComponent>();
                    
                    for (auto childUUID : h.m_Childrens)
                        stack.push_back(childUUID);
                }
            }

            for (auto itU = order.rbegin(); itU != order.rend(); ++itU)
            {
                auto itc = m_EntityMap.find(*itU);
                if (itc == m_EntityMap.end()) continue;

                Entity e{ itc->second, m_Registry.get() };
                if (e.IsValid()) DestroyEntityNow(e);
                else             m_EntityMap.erase(*itU);
            }
        }
    }

    void Scene::DestroyEntityNow(Entity entity)
    {
        if (entity == Entity::Null() || !entity.IsValid())
            return;

        if (entity.HasComponent<TagComponent>())
        {
            const std::string& tag = entity.GetComponent<TagComponent>().Tag;
            UnregisterEntityName(tag);
        }

        if (entity.HasComponent<HierarchyComponent>())
        {
            auto& h = entity.GetComponent<HierarchyComponent>();
        }

        UUID uuid = entity.GetUUID();
        if (m_PrimaryCameraUUID == uuid)
            m_PrimaryCameraUUID = UUID::Null();

        m_EntityMap.erase(uuid);

        if (entity.IsValid())
        {
            m_Registry->DestroyEntity(entity);
        }
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

        auto view = m_Registry->GetRegistry().view<AnimationComponent>();
        for (auto entity : view)
        {
            auto& anim = view.get<AnimationComponent>(entity);
            anim.Update(deltaTime);
        }

        for (auto [e, tr, pc] : m_Registry->GetRegistry().group<TransformComponent, ParticleComponent>().each())
        {
            //glm::vec3 emitterPos = tr.GetGlobalTransform() * glm::vec4(pc.m_EmitterOffset, 1.0f);
            //pc.Update(deltaTime, emitterPos);
            pc.Update(deltaTime);
        }

        if (m_OnRuntime)
            UpdateRuntime(deltaTime);
    }

    void Scene::UpdateRuntime(double deltaTime)
    {
        PhysicEngine::Instance().Step(deltaTime);

        auto view = m_Registry->GetRegistry().view<RigidBodyComponent>();
        for (auto entity : view)
        {
            auto& rigid = view.get<RigidBodyComponent>(entity);
            rigid.Update(deltaTime);
        }

        Renderer::Instance().m_SceneData.m_ScriptSystem->Update(deltaTime);

        if (Input::IsKeyJustPressed(Key::Escape))
        {
            Application::Get().GetWindow().SetInputMode(false, false);
		}
    }

    void Scene::OnRuntimeStart()
    {
        m_OnRuntime = true;

        Renderer::Instance().BeginScene(*this);

        RebuildEntityCaches();

        UpdatePrimaryCameraCache();

        Renderer::Instance().m_SceneData.m_ScriptSystem->Start();

        Application::Get().GetWindow().SetInputMode(true, true);
        Application::Get().GetWindow().SetCursorVisibility(false);
    }

    void Scene::OnRuntimeStop()
    {
        m_OnRuntime = false;

        Renderer::Instance().m_SceneData.m_ScriptSystem->Stop();

        Application::Get().GetWindow().SetInputMode(false, false);
        Application::Get().GetWindow().SetCursorVisibility(true);
    }

    void Scene::ClearEntities()
    {
        std::vector<UUID> uuids;
        uuids.reserve(m_EntityMap.size());
        for (auto& [uuid, handle] : m_EntityMap) uuids.push_back(uuid);
        for (UUID uuid : uuids)
            DestroyEntity(uuid);

        ProcessEntityDestructions();
        m_PrimaryCameraUUID = UUID::Null();
    }

    void Scene::RebuildEntityCaches()
    {
        m_EntityMap.clear();
        m_NameMap.clear();

        auto& reg = m_Registry->GetRegistry();
        auto view = reg.view<IDComponent, TagComponent>();

        for (auto e : view)
        {
            auto& id = view.get<IDComponent>(e);
            auto& tag = view.get<TagComponent>(e);

            m_EntityMap[id.ID] = e;
            RegisterEntityName(tag.Tag, e);
        }

        UpdatePrimaryCameraCache();
    }

    std::optional<Entity> Scene::GetPrimaryCameraEntity() const
    {
        if (m_PrimaryCameraUUID != UUID::Null())
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
        m_PrimaryCameraUUID = UUID::Null();
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
