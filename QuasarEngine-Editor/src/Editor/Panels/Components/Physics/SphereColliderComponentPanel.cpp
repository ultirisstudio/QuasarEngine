#include "SphereColliderComponentPanel.h"

#include <imgui/imgui.h>
#include <glm/glm.hpp>

#include <QuasarEngine/Entity/Entity.h>
#include <QuasarEngine/Entity/Components/TransformComponent.h>
#include <QuasarEngine/Entity/Components/Physics/SphereColliderComponent.h>

namespace QuasarEngine
{
    static inline float ClampMin(float v, float mn) { return v < mn ? mn : v; }
    static inline float Max3(float a, float b, float c) { return a > b ? (a > c ? a : c) : (b > c ? b : c); }

    void SphereColliderComponentPanel::Render(Entity entity)
    {
        if (!entity.HasComponent<SphereColliderComponent>()) return;
        auto& cc = entity.GetComponent<SphereColliderComponent>();

        if (!ImGui::TreeNodeEx("Sphere Collider", ImGuiTreeNodeFlags_DefaultOpen, "Sphere Collider")) return;

        if (ImGui::BeginPopupContextItem())
        {
            if (ImGui::MenuItem("Delete Component"))
                entity.RemoveComponent<SphereColliderComponent>();
            ImGui::EndPopup();
        }

        bool sizeChanged = false;

        if (ImGui::Checkbox("Use Entity Scale", &cc.m_UseEntityScale))
            sizeChanged = true;

        {
            float baseRadius = cc.m_Radius;
            if (ImGui::DragFloat("Base Radius", &baseRadius, 0.01f, 0.0001f, 1e6f))
            {
                cc.m_Radius = ClampMin(baseRadius, 0.0001f);
                sizeChanged = true;
            }
        }

        if (sizeChanged)
            cc.UpdateColliderSize();

        float scaleFactor = 1.f;
        if (cc.m_UseEntityScale && entity.HasComponent<TransformComponent>())
        {
            const auto& s = entity.GetComponent<TransformComponent>().Scale;
            scaleFactor = Max3(std::abs(s.x), std::abs(s.y), std::abs(s.z));
        }
        const float rEff = ClampMin(cc.m_Radius * scaleFactor, 0.0001f);
        const float volume = (4.0f / 3.0f) * 3.14159265358979323846f * rEff * rEff * rEff;

        ImGui::SeparatorText("Shape");
        ImGui::Text("Computed Radius: %.6f m", rEff);
        ImGui::Text("Volume:          %.6f m^3", volume);

        ImGui::SeparatorText("Material / Mass");
        bool matChanged = false;
        matChanged |= ImGui::DragFloat("Mass (kg)", &cc.mass, 0.1f, 0.0f, 1e9f);
        matChanged |= ImGui::DragFloat("Friction", &cc.friction, 0.01f, 0.0f, 10.0f);
        matChanged |= ImGui::DragFloat("Bounciness", &cc.bounciness, 0.01f, 0.0f, 1.0f);
        if (matChanged)
            cc.UpdateColliderMaterial();

        if (ImGui::Button("Rebuild Shape"))
            cc.UpdateColliderSize();

        ImGui::TreePop();
    }
}