#pragma once

#include <string>
#include <vector>
#include <entt.hpp>

#include <QuasarEngine/ECS/Registry.h>
#include <QuasarEngine/Entity/Components/IDComponent.h>
#include <QuasarEngine/Entity/Components/TagComponent.h>

namespace QuasarEngine
{
	class Entity
	{
	public:
		Entity() = default;
		Entity(entt::entity handle, Registry* registry);
		Entity(const Entity& other) = default;

		static Entity Null() { return {entt::null, nullptr}; }

		template<typename T, typename... Args>
		T& AddComponent(Args&&... args)
		{
			if (HasComponent<T>())
				return GetComponent<T>();

			T& component = m_Registry->GetRegistry().emplace<T>(m_EntityHandle, std::forward<Args>(args)...);
			component.entt_entity = m_EntityHandle;
			component.registry = m_Registry;
			return component;
		}

		template<typename T, typename... Args>
		T& AddOrReplaceComponent(Args&&... args)
		{
			T& component = m_Registry->GetRegistry().emplace_or_replace<T>(m_EntityHandle, std::forward<Args>(args)...);
			component.entt_entity = m_EntityHandle;
			component.registry = m_Registry;
			return component;
		}

		template<typename T>
		T& GetComponent()
		{
			return m_Registry->GetRegistry().get<T>(m_EntityHandle);
		}

		template<typename T>
		bool HasComponent()
		{
			return m_Registry->GetRegistry().all_of<T>(m_EntityHandle);
		}

		template<typename T>
		void RemoveComponent()
		{
			m_Registry->GetRegistry().remove<T>(m_EntityHandle);
		}

		operator bool() const { return m_EntityHandle != entt::null; }
		operator entt::entity() const { return m_EntityHandle; }
		operator uint32_t() const { return (uint32_t)m_EntityHandle; }

		UUID GetUUID() { return GetComponent<IDComponent>().ID; }
		std::string GetName() { return GetComponent<TagComponent>().Tag; }

		bool Entity::IsValid() const {
			return m_Registry && m_Registry->IsValid(m_EntityHandle);
		}

		bool operator==(const Entity& other) const
		{
			return m_EntityHandle == other.m_EntityHandle;
		}

		bool operator!=(const Entity& other) const
		{
			return !(*this == other);
		}
	private:
		entt::entity m_EntityHandle{ entt::null };
		Registry* m_Registry = nullptr;
	};

	/*struct EntityMapping
	{
		size_t operator()(const Entity& k)const
		{
			return std::hash<int>()(k.GetUUID());
		}

		bool operator()(const Entity& a, const Entity& b)const
		{
			return a.GetUUID() == b.GetUUID();
		}
	};*/
}