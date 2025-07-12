#pragma once

#include <QuasarEngine/Entity/Component.h>
#include <QuasarEngine/Core/UUID.h>

namespace QuasarEngine
{
	class IDComponent : public Component
	{
	public:
		UUID ID;

		IDComponent() = default;
		IDComponent(const IDComponent&) = default;
		IDComponent(UUID id) : ID(id) {}
	};
}