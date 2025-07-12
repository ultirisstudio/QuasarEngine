#include "SphereColliderComponentPanel.h"

#include <imgui/imgui.h>
#include <glm/gtc/type_ptr.hpp>

#include <QuasarEngine/Entity/Entity.h>
#include <QuasarEngine/Entity/Components/Physics/SphereColliderComponent.h>
#include <QuasarEngine/Physic/Shape/SphereShape.h>
#include <QuasarEngine/Physic/Shape/ProxyShape.h>
#include <QuasarEngine/Physic/Collision/Collider.h>

namespace QuasarEngine
{
	void SphereColliderComponentPanel::Render(Entity entity)
	{
		if (!entity.HasComponent<SphereColliderComponent>())
			return;

		auto& cc = entity.GetComponent<SphereColliderComponent>();

		if (ImGui::TreeNodeEx("Sphere Collider", ImGuiTreeNodeFlags_DefaultOpen, "Sphere Collider"))
		{
			if (ImGui::BeginPopupContextItem())
			{
				if (ImGui::MenuItem("Delete Component"))
				{
					entity.RemoveComponent<SphereColliderComponent>();
				}
				ImGui::EndPopup();
			}

			ImGui::Checkbox("Use entity scale", &cc.UseEntityScale());

			if (ImGui::DragFloat("Base Radius", &cc.Radius(), 0.05f, 0.01f, 1000.0f))
			{
				cc.UpdateColliderSize();
			}

			if (Collider* collider = cc.GetCollider()) {
				const auto& proxyShapes = collider->GetProxyShapes();

				if (!proxyShapes.empty() && proxyShapes[0]) {
					if (CollisionShape* shape = proxyShapes[0]->GetCollisionShape()) {
						if (auto* sphere = dynamic_cast<SphereShape*>(shape)) {
							ImGui::Text("Computed Radius: %.2f", sphere->GetRadius());
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