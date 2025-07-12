#pragma once

#include <QuasarEngine/Entity/Component.h>

namespace QuasarEngine
{
	class PrimitiveColliderComponent : public Component
	{
	public:
		float mass;
		float friction;
		float bounciness;

		PrimitiveColliderComponent();

		virtual void Init() = 0;
		virtual void UpdateColliderMaterial() = 0;
		virtual void UpdateColliderSize() = 0;
	};
}