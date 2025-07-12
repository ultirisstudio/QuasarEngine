#pragma once

#include <QuasarEngine/Entity/Component.h>
#include <QuasarEngine/Resources/Lights/PointLight.h>
#include <QuasarEngine/Resources/Lights/DirectionalLight.h>

namespace QuasarEngine
{
	class LightComponent : public Component
	{
	public:
		enum class LightType
		{
			NONE = 0,
			DIRECTIONAL = 1,
			POINT = 2
		};

		PointLight point_light;
		DirectionalLight directional_light;

		const char* item_type = "Directional Light";
		LightType lightType = LightType::DIRECTIONAL;

		LightComponent();
		LightComponent(LightType type);

		void SetType(LightType type);
	};
}