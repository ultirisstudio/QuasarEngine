#pragma once

#include <QuasarEngine/Entity/Component.h>

namespace QuasarEngine
{
	class MeshColliderComponent : public Component
	{
	private:
		void GenerateConcaveMesh();
		void GenerateConvexMesh();
	public:
		MeshColliderComponent();

		void Generate();

		bool m_IsConvex = false;
	};
}