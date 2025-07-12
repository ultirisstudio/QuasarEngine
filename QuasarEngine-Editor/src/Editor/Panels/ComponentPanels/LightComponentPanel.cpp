#include "LightComponentPanel.h"

#include <imgui/imgui.h>
#include "glm/gtc/type_ptr.hpp"

#include <QuasarEngine/Entity/Entity.h>
#include <QuasarEngine/Entity/Components/LightComponent.h>

namespace QuasarEngine
{
	void LightComponentPanel::Render(Entity entity)
	{
		if (entity.HasComponent<LightComponent>())
		{
			auto& lc = entity.GetComponent<LightComponent>();

			if (ImGui::TreeNodeEx("Light", ImGuiTreeNodeFlags_DefaultOpen, "Light"))
			{
				if (ImGui::BeginPopupContextItem())
				{
					if (ImGui::MenuItem("Delete Component"))
					{
						entity.RemoveComponent<LightComponent>();
					}
					ImGui::EndPopup();
				}

				const char* items[] = { "Directional Light", "Point Light" };
				const char* current_item = lc.item_type;

				if (ImGui::BeginCombo("##combolighttype", current_item))
				{
					for (int n = 0; n <= 1; n++)
					{
						bool is_selected = (current_item == items[n]);
						if (ImGui::Selectable(items[n], is_selected))
						{
							if (items[n] == "Directional Light")
							{
								lc.SetType(LightComponent::LightType::DIRECTIONAL);
							}
							else if (items[n] == "Point Light")
							{
								lc.SetType(LightComponent::LightType::POINT);
							}
						}
						if (is_selected)
							ImGui::SetItemDefaultFocus();
					}
					ImGui::EndCombo();
				}

				if (lc.lightType == LightComponent::LightType::DIRECTIONAL)
				{
					ImGui::Text("Directional light color: ");
					ImGui::ColorEdit3("##dirlightcolor", glm::value_ptr(lc.directional_light.color));

					ImGui::Text("Directional light power: ");
					ImGui::SliderFloat("##dirpower", &lc.directional_light.power, 0.0f, 100.0f);
				}
				else if (lc.lightType == LightComponent::LightType::POINT)
				{
					ImGui::Text("Point light color: ");
					ImGui::ColorEdit3("##pointlightcolor", glm::value_ptr(lc.point_light.color));

					ImGui::Text("Point light attenuation: ");
					ImGui::SliderFloat("##pointattenuation", &lc.point_light.attenuation, 0.001f, 10.0f);

					ImGui::Text("Point light power: ");
					ImGui::SliderFloat("##pointpower", &lc.point_light.power, 0.0f, 100.0f);
				}

				ImGui::TreePop();
			}
		}
	}
}