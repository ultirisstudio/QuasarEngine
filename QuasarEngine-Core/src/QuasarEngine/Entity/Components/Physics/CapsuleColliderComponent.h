#pragma once

#include <QuasarEngine/Entity/Components/Physics/PrimitiveColliderComponent.h>

namespace QuasarEngine
{
	class CapsuleColliderComponent : public PrimitiveColliderComponent
	{
	public:
		CapsuleColliderComponent();
		~CapsuleColliderComponent();

		void Init() override;
		void UpdateColliderMaterial() override;
		void UpdateColliderSize() override;

		float m_Radius = 0.5f;
		float m_Height = 1.0f;
	};
}