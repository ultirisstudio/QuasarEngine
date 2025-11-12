#pragma once

#include <Editor/Panels/IComponentPanel.h>

namespace QuasarEngine
{
	class Entity;

	class CameraComponentPanel : public IComponentPanel
	{
	public:
		CameraComponentPanel() = default;
		~CameraComponentPanel() = default;

		void Render(Entity entity) override;
		const char* Name() const override { return ""; }
	};
}
