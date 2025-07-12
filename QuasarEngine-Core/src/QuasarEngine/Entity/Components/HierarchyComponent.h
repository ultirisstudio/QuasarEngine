#pragma once

#include <vector>
#include <QuasarEngine/Entity/Component.h>
#include <QuasarEngine/Core/UUID.h>

namespace QuasarEngine
{
	class HierarchyComponent : public Component
	{
	public:
		std::vector<UUID> m_Childrens;
		UUID m_Parent = UUID::Null();

		HierarchyComponent() = default;
		HierarchyComponent(const HierarchyComponent&) = default;

		void AddChild(UUID parent, UUID child);
	};
}