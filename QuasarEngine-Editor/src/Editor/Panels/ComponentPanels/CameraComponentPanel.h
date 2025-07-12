#pragma once


#include "../../SceneManager.h"

namespace QuasarEngine
{
	class Entity;

	class CameraComponentPanel
	{
	public:
		CameraComponentPanel() = default;
		~CameraComponentPanel() = default;

		void Render(Entity entity, Scene& scene);
	};
}
