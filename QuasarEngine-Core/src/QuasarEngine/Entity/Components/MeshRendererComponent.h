#pragma once

#include <QuasarEngine/Entity/Component.h>

namespace QuasarEngine
{
	class MeshRendererComponent : public Component
	{
	public:
		MeshRendererComponent() = default;
		~MeshRendererComponent() = default;

		bool m_Rendered = true;
	};
}