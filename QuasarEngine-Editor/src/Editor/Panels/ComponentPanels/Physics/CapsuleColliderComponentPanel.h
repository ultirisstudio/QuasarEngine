#pragma once

namespace QuasarEngine
{
	class Entity;

	class CapsuleColliderComponentPanel
	{
	public:
		CapsuleColliderComponentPanel() = default;
		~CapsuleColliderComponentPanel() = default;

		void Render(Entity entity);
	};
}