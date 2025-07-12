#pragma once

namespace QuasarEngine
{
	class Entity;

	class RigidBodyComponentPanel
	{
	public:
		RigidBodyComponentPanel() = default;
		~RigidBodyComponentPanel() = default;

		void Render(Entity entity);
	};
}