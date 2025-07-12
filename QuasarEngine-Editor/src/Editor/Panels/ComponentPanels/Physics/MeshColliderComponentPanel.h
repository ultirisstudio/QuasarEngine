#pragma once

namespace QuasarEngine
{
	class Entity;

	class MeshColliderComponentPanel
	{
	public:
		MeshColliderComponentPanel() = default;
		~MeshColliderComponentPanel() = default;

		void Render(Entity entity);
	};
}