#include "CapsuleColliderComponentPanel.h"

#include <imgui/imgui.h>
#include <glm/glm.hpp>

#include <QuasarEngine/Entity/Entity.h>
#include <QuasarEngine/Entity/Components/TransformComponent.h>
#include <QuasarEngine/Entity/Components/Physics/CapsuleColliderComponent.h>

namespace QuasarEngine
{
    static inline float ClampMin(float v, float mn) { return v < mn ? mn : v; }
    static inline float Max3(float a, float b, float c) { return a > b ? (a > c ? a : c) : (b > c ? b : c); }

    void CapsuleColliderComponentPanel::Render(Entity entity)
    {
        if (!entity.HasComponent<CapsuleColliderComponent>()) return;
        auto& cc = entity.GetComponent<CapsuleColliderComponent>();

        if (!ImGui::TreeNodeEx("Capsule Collider", ImGuiTreeNodeFlags_DefaultOpen, "Capsule Collider")) return;

        if (ImGui::BeginPopupContextItem())
        {
            if (ImGui::MenuItem("Delete Component"))
                entity.RemoveComponent<CapsuleColliderComponent>();
            ImGui::EndPopup();
        }

        bool sizeChanged = false;

        if (ImGui::Checkbox("Use Entity Scale", &cc.m_UseEntityScale))
            sizeChanged = true;

        const char* axisItems[] = { "X", "Y", "Z" };
        int axisIdx = static_cast<int>(cc.m_Axis);
        if (ImGui::Combo("Axis", &axisIdx, axisItems, IM_ARRAYSIZE(axisItems)))
        {
            cc.m_Axis = static_cast<CapsuleColliderComponent::Axis>(axisIdx);
            sizeChanged = true;
        }

        {
            float r = cc.m_Radius;
            if (ImGui::DragFloat("Base Radius", &r, 0.01f, 0.0001f, 1e6f))
            {
                cc.m_Radius = ClampMin(r, 0.0001f);
                sizeChanged = true;
            }
        }
        {
            float h = cc.m_Height;
            if (ImGui::DragFloat("Base Height", &h, 0.01f, 0.0001f, 1e6f))
            {
                cc.m_Height = ClampMin(h, 0.0001f);
                sizeChanged = true;
            }
        }

        if (sizeChanged)
            cc.UpdateColliderSize();

        float scaleRadius = 1.f, scaleHeight = 1.f;
        if (cc.m_UseEntityScale && entity.HasComponent<TransformComponent>())
        {
            const auto& s = entity.GetComponent<TransformComponent>().Scale;
            switch (cc.m_Axis)
            {
            case CapsuleColliderComponent::Axis::X:
                scaleHeight = std::abs(s.x);
                scaleRadius = Max3(std::abs(s.y), std::abs(s.z), 1e-4f);
                break;
            case CapsuleColliderComponent::Axis::Y:
                scaleHeight = std::abs(s.y);
                scaleRadius = Max3(std::abs(s.x), std::abs(s.z), 1e-4f);
                break;
            case CapsuleColliderComponent::Axis::Z:
                scaleHeight = std::abs(s.z);
                scaleRadius = Max3(std::abs(s.x), std::abs(s.y), 1e-4f);
                break;
            }
        }

        const float rEff = ClampMin(cc.m_Radius * scaleRadius, 0.0001f);
        const float hEff = ClampMin(cc.m_Height * scaleHeight, 0.0001f);
        const float cylH = hEff;
        const float volume = 3.14159265358979323846f * rEff * rEff * cylH
            + (4.0f / 3.0f) * 3.14159265358979323846f * rEff * rEff * rEff;

        ImGui::SeparatorText("Derived");
        ImGui::Text("Effective Radius: %.6f m", rEff);
        ImGui::Text("Effective Height (cylinder): %.6f m", hEff);
        ImGui::Text("Volume: %.6f m^3", volume);

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
