#pragma once

namespace QuasarEngine
{
	class Entity;

	class MeshRendererComponentPanel
	{
	public:
		MeshRendererComponentPanel() = default;
		~MeshRendererComponentPanel() = default;
		void Render(Entity entity);
	};
}
