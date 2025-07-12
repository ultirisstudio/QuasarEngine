#pragma once

#include <QuasarEngine/Entity/Component.h>
#include <QuasarEngine/Resources/Materials/Material.h>

namespace QuasarEngine
{
	class MaterialComponent : public Component
	{
	private:
		std::shared_ptr<Material> m_Material;
	public:
		MaterialComponent();
		MaterialComponent(const MaterialSpecification& specification);
		~MaterialComponent();

		Material& GetMaterial();
	};
}