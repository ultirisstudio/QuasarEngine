#pragma once

#include <memory>
#include <string>

namespace QuasarEngine
{
	class Entity;
	class Material;

	struct MaterialSpecification;
	enum TextureType;

	class MaterialComponentPanel
	{
	public:
		MaterialComponentPanel();
		~MaterialComponentPanel() = default;

		void Render(Entity entity);
	};
}
