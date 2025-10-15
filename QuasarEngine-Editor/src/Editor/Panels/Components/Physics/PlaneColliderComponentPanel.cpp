#include "PlaneColliderComponentPanel.h"

#include <imgui/imgui.h>
#include <glm/glm.hpp>
#include <glm/gtc/quaternion.hpp>

#include <QuasarEngine/Entity/Entity.h>
#include <QuasarEngine/Entity/Components/TransformComponent.h>
#include <QuasarEngine/Entity/Components/Physics/PlaneColliderComponent.h>
#include <QuasarEngine/Entity/Components/Physics/RigidBodyComponent.h>

namespace QuasarEngine
{
    static inline glm::vec3 NormalizeSafe(const glm::vec3& v, float eps = 1e-6f)
    {
        float len2 = glm::dot(v, v);
        if (len2 < eps * eps) return { 0.f, 1.f, 0.f };
        return v / std::sqrt(len2);
    }

    void PlaneColliderComponentPanel::Render(Entity entity)
    {
        if (!entity.HasComponent<PlaneColliderComponent>()) return;
        auto& cc = entity.GetComponent<PlaneColliderComponent>();

        if (!ImGui::TreeNodeEx("Plane Collider", ImGuiTreeNodeFlags_DefaultOpen, "Plane Collider"))
            return;

        if (ImGui::BeginPopupContextItem())
        {
            if (ImGui::MenuItem("Delete Component"))
                entity.RemoveComponent<PlaneColliderComponent>();
            ImGui::EndPopup();
        }

        if (entity.HasComponent<RigidBodyComponent>())
        {
            auto& rb = entity.GetComponent<RigidBodyComponent>();
            if (rb.GetDynamic())
            {
                ImGui::PushStyleColor(ImGuiCol_Text, IM_COL32(255, 120, 120, 255));
                ImGui::TextUnformatted("Un plane collider doit être attaché à un acteur STATIC (pas Dynamic/Kinematic).");
                ImGui::PopStyleColor();
                if (ImGui::Button("Convertir le RigidBody en STATIC"))
                {
                    rb.bodyTypeString = "STATIC";
                    rb.UpdateBodyType();
                }
            }
        }

        bool changedPose = false;

        if (ImGui::Checkbox("Use Entity Orientation (+Y as normal)", &cc.m_UseEntityOrientation))
            changedPose = true;

        if (ImGui::DragFloat("Distance (m)", &cc.m_Distance, 0.01f, -1e6f, 1e6f))
            changedPose = true;

        if (!cc.m_UseEntityOrientation)
        {
            glm::vec3 n = cc.m_Normal;
            if (ImGui::DragFloat3("Normal", &n.x, 0.01f, -1.f, 1.f))
            {
                cc.m_Normal = NormalizeSafe(n);
                changedPose = true;
            }
        }
        else
        {
            if (entity.HasComponent<TransformComponent>())
            {
                const auto& t = entity.GetComponent<TransformComponent>();
                glm::quat q = glm::quat(t.Rotation);
                glm::vec3 nWorld = q * glm::vec3(0, 1, 0);
                ImGui::Text("Entity +Y normal: (%.3f, %.3f, %.3f)", nWorld.x, nWorld.y, nWorld.z);
            }
        }

        if (changedPose)
            cc.UpdateColliderSize();

        ImGui::SeparatorText("Material");
        bool matChanged = false;
        matChanged |= ImGui::DragFloat("Friction", &cc.friction, 0.01f, 0.0f, 10.0f);
        matChanged |= ImGui::DragFloat("Bounciness", &cc.bounciness, 0.01f, 0.0f, 1.0f);
        if (matChanged)
            cc.UpdateColliderMaterial();

        if (ImGui::Button("Rebuild Shape"))
            cc.UpdateColliderSize();

        ImGui::TreePop();
    }
}