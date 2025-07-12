#include "RigidBodyComponentPanel.h"

#include <imgui/imgui.h>

#include <QuasarEngine/Entity/Entity.h>
#include <QuasarEngine/Entity/Components/Physics/RigidBodyComponent.h>

namespace QuasarEngine
{
	void RigidBodyComponentPanel::Render(Entity entity)
	{
		if (entity.HasComponent<RigidBodyComponent>())
		{
			auto& rbc = entity.GetComponent<RigidBodyComponent>();

			if (ImGui::TreeNodeEx("RigidBody", ImGuiTreeNodeFlags_DefaultOpen, "RigidBody"))
			{
				if (ImGui::BeginPopupContextItem())
				{
					if (ImGui::MenuItem("Delete Component"))
					{
						entity.RemoveComponent<RigidBodyComponent>();
					}
					ImGui::EndPopup();
				}

				if (ImGui::Checkbox("Enable Gravity", &rbc.enableGravity))
				{
					rbc.UpdateEnableGravity();
				}

				const char* items[] = { "STATIC", "KINEMATIC", "DYNAMIC" };

				if (ImGui::BeginCombo("##combo", rbc.bodyTypeString.data()))
				{
					for (int n = 0; n < IM_ARRAYSIZE(items); n++)
					{
						bool is_selected = (rbc.bodyTypeString == items[n]);
						if (ImGui::Selectable(items[n], is_selected))
						{
							rbc.bodyTypeString = items[n];
							rbc.UpdateBodyType();
						}

						if (is_selected)
							ImGui::SetItemDefaultFocus();
					}
					ImGui::EndCombo();
				}

				ImGui::Text("Linear Axis Factor");
				ImGui::Text("X: "); ImGui::SameLine();
				if (ImGui::Checkbox("##LX", &rbc.m_LinearAxisFactorX))
				{
					rbc.UpdateLinearAxisFactor();
				}
				ImGui::SameLine();
				ImGui::Text("Y: "); ImGui::SameLine();
				if (ImGui::Checkbox("##LY", &rbc.m_LinearAxisFactorY))
				{
					rbc.UpdateLinearAxisFactor();
				}
				ImGui::SameLine();
				ImGui::Text("Z: "); ImGui::SameLine();
				if (ImGui::Checkbox("##LZ", &rbc.m_LinearAxisFactorZ))
				{
					rbc.UpdateLinearAxisFactor();
				}

				ImGui::Text("Angular Axis Factor");
				ImGui::Text("X: "); ImGui::SameLine();
				if (ImGui::Checkbox("##AX", &rbc.m_AngularAxisFactorX))
				{
					rbc.UpdateAngularAxisFactor();
				}
				ImGui::SameLine();
				ImGui::Text("Y: "); ImGui::SameLine();
				if (ImGui::Checkbox("##AY", &rbc.m_AngularAxisFactorY))
				{
					rbc.UpdateAngularAxisFactor();
				}
				ImGui::SameLine();
				ImGui::Text("Z: "); ImGui::SameLine();
				if (ImGui::Checkbox("##AZ", &rbc.m_AngularAxisFactorZ))
				{
					rbc.UpdateAngularAxisFactor();
				}

				ImGui::Text("Damping");
				bool dampingChanged = false;

				dampingChanged |= ImGui::DragFloat("Linear Damping", &rbc.linearDamping, 0.01f, 0.0f, 1.0f);
				dampingChanged |= ImGui::DragFloat("Angular Damping", &rbc.angularDamping, 0.01f, 0.0f, 1.0f);

				if (dampingChanged) {
					rbc.UpdateDamping();
				}

				ImGui::TreePop();
			}
		}
	}
}