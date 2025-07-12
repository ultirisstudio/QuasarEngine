#include "BoxColliderComponentPanel.h"

#include <imgui/imgui.h>
#include <glm/gtc/type_ptr.hpp>

#include <QuasarEngine/Entity/Entity.h>
#include <QuasarEngine/Entity/Components/Physics/BoxColliderComponent.h>
#include <QuasarEngine/Physic/Shape/BoxShape.h>
#include <QuasarEngine/Physic/Shape/ProxyShape.h>
#include <QuasarEngine/Physic/Collision/Collider.h>

namespace QuasarEngine
{
	void BoxColliderComponentPanel::Render(Entity entity)
	{
		if (!entity.HasComponent<BoxColliderComponent>())
			return;

		auto& cc = entity.GetComponent<BoxColliderComponent>();

		if (ImGui::TreeNodeEx("Box Collider", ImGuiTreeNodeFlags_DefaultOpen, "Box Collider"))
		{
			if (ImGui::BeginPopupContextItem())
			{
				if (ImGui::MenuItem("Delete Component"))
				{
					entity.RemoveComponent<BoxColliderComponent>();
				}
				ImGui::EndPopup();
			}

			ImGui::Checkbox("Use entity scale", &cc.m_UseEntityScale);
			if (!cc.m_UseEntityScale)
			{
				if (ImGui::DragFloat3("Manual Size", glm::value_ptr(cc.m_Size), 0.1f, 0.01f, 1000.0f))
				{
					cc.UpdateColliderSize();
				}
			}

			if (Collider* collider = cc.GetCollider()) {
				const auto& proxyShapes = collider->GetProxyShapes();

				if (!proxyShapes.empty() && proxyShapes[0]) {
					if (CollisionShape* shape = proxyShapes[0]->GetCollisionShape()) {
						if (auto* box = dynamic_cast<BoxShape*>(shape)) {
							glm::vec3 halfExtents = box->GetHalfExtents();
							ImGui::Text("Half Extents: %.2f, %.2f, %.2f", halfExtents.x, halfExtents.y, halfExtents.z);
						}
					}
				}
			}

			if (ImGui::TreeNodeEx("Material", ImGuiTreeNodeFlags_DefaultOpen))
			{
				bool updated = false;

				ImGui::Text("Mass");
				updated |= ImGui::DragFloat("##Mass", &cc.mass, 1.0f, 0.0f, 1000.0f);

				ImGui::Text("Friction");
				updated |= ImGui::DragFloat("##Friction", &cc.friction, 0.05f, 0.0f, 10.0f);

				ImGui::Text("Bounciness");
				updated |= ImGui::DragFloat("##Bounciness", &cc.bounciness, 0.05f, 0.0f, 1.0f);

				if (updated) {
					cc.UpdateColliderMaterial();
				}

				ImGui::TreePop();
			}

			ImGui::TreePop();
		}
	}
}