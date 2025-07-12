#pragma once

#include <entt.hpp>

namespace QuasarEngine
{
	class Registry
	{
	private:
		std::unique_ptr<entt::registry> m_Registry;

	public:
		Registry();
		~Registry() = default;

		inline entt::registry& GetRegistry() { return *m_Registry; }
		inline entt::entity CreateEntity() { return m_Registry->create(); }
		inline void DestroyEntity(entt::entity entity) { m_Registry->destroy(entity); }
		inline void ClearRegistry() { m_Registry->clear(); }

		inline bool IsValid(entt::entity entity) const
		{
			return m_Registry->valid(entity);
		}

		template <typename TContext>
		inline TContext AddToContext(TContext context)
		{
			return m_Registry->ctx().emplace<TContext>(context);
		}

		template <typename TContext>
		inline TContext& GetContext()
		{
			return m_Registry->ctx().get<TContext>();
		}

		template <typename TContext>
		inline TContext* TryGetContext()
		{
			return m_Registry->ctx().find<TContext>();
		}

		template <typename TContext>
		bool RemoveContext()
		{
			return m_Registry->ctx().erase<TContext>();
		}

		template <typename TContext>
		bool HasContext()
		{
			return m_Registry->ctx().contains<TContext>();
		}
	};
}