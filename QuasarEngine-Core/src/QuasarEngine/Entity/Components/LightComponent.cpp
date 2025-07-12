#include <qepch.h>
#include "LightComponent.h"

#include <QuasarEngine/Renderer/Renderer.h>

namespace QuasarEngine
{
	LightComponent::LightComponent()
	{
	}

	LightComponent::LightComponent(LightType type)
	{
		SetType(type);
	}

	void LightComponent::SetType(LightType type)
	{
		lightType = type;

		if (type == LightType::POINT)
		{
			item_type = "Point Light";
			lightType = LightType::POINT;
		}
		else if (type == LightType::DIRECTIONAL)
		{
			item_type = "Directional Light";
			lightType = LightType::DIRECTIONAL;
		}
	}
}