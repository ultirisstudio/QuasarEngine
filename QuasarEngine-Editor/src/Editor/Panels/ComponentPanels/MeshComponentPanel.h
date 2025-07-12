#pragma once

namespace QuasarEngine
{
	class Entity;

	class MeshComponentPanel
	{
	public:
		MeshComponentPanel() = default;
		~MeshComponentPanel() = default;
		void Render(Entity entity);
	};
}
