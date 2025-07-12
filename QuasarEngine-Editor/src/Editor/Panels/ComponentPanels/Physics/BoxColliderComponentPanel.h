#pragma once

namespace QuasarEngine
{
	class Entity;

	class BoxColliderComponentPanel
	{
	public:
		BoxColliderComponentPanel() = default;
		~BoxColliderComponentPanel() = default;

		void Render(Entity entity);
	};
}