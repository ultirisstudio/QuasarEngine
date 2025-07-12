#pragma once

namespace QuasarEngine
{
	class Entity;

	class SphereColliderComponentPanel
	{
	public:
		SphereColliderComponentPanel() = default;
		~SphereColliderComponentPanel() = default;

		void Render(Entity entity);
	};
}