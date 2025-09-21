#include "RigidBodyComponentPanel.h"

#include <imgui/imgui.h>
#include <glm/glm.hpp>

#include <QuasarEngine/Entity/Entity.h>
#include <QuasarEngine/Entity/Components/Physics/RigidBodyComponent.h>

namespace QuasarEngine
{
    static inline void HelpMarker(const char* text)
    {
        ImGui::SameLine();
        ImGui::TextDisabled("(?)");
        if (ImGui::IsItemHovered())
        {
            ImGui::BeginTooltip();
            ImGui::PushTextWrapPos(ImGui::GetFontSize() * 32.0f);
            ImGui::TextUnformatted(text);
            ImGui::PopTextWrapPos();
            ImGui::EndTooltip();
        }
    }

    void RigidBodyComponentPanel::Render(Entity entity)
    {
        if (!entity.HasComponent<RigidBodyComponent>()) return;
        auto& rbc = entity.GetComponent<RigidBodyComponent>();

        if (!ImGui::TreeNodeEx("RigidBody", ImGuiTreeNodeFlags_DefaultOpen, "RigidBody")) return;

        if (ImGui::BeginPopupContextItem())
        {
            if (ImGui::MenuItem("Delete Component"))
                entity.RemoveComponent<RigidBodyComponent>();
            ImGui::EndPopup();
        }

        if (ImGui::Checkbox("Enable Gravity", &rbc.enableGravity))
            rbc.UpdateEnableGravity();

        const char* items[] = { "STATIC", "KINEMATIC", "DYNAMIC" };
        if (ImGui::BeginCombo("Body Type", rbc.bodyTypeString.c_str()))
        {
            for (int i = 0; i < IM_ARRAYSIZE(items); ++i)
            {
                bool selected = (rbc.bodyTypeString == items[i]);
                if (ImGui::Selectable(items[i], selected))
                {
                    rbc.bodyTypeString = items[i];
                    rbc.UpdateBodyType();
                }
                if (selected) ImGui::SetItemDefaultFocus();
            }
            ImGui::EndCombo();
        }

        ImGui::SeparatorText("Freeze (Axis Locks)");

        bool anyLin = false;
        anyLin |= ImGui::Checkbox("Freeze X (Linear)", &rbc.m_LinearAxisFactorX);
        ImGui::SameLine();
        anyLin |= ImGui::Checkbox("Freeze Y (Linear)", &rbc.m_LinearAxisFactorY);
        ImGui::SameLine();
        anyLin |= ImGui::Checkbox("Freeze Z (Linear)", &rbc.m_LinearAxisFactorZ);
        if (anyLin) rbc.UpdateLinearAxisFactor();

        bool anyAng = false;
        anyAng |= ImGui::Checkbox("Freeze X (Angular)", &rbc.m_AngularAxisFactorX);
        ImGui::SameLine();
        anyAng |= ImGui::Checkbox("Freeze Y (Angular)", &rbc.m_AngularAxisFactorY);
        ImGui::SameLine();
        anyAng |= ImGui::Checkbox("Freeze Z (Angular)", &rbc.m_AngularAxisFactorZ);
        if (anyAng) rbc.UpdateAngularAxisFactor();

        ImGui::SeparatorText("Damping");
        bool dampingChanged = false;
        dampingChanged |= ImGui::DragFloat("Linear Damping", &rbc.linearDamping, 0.01f, 0.0f, 10.0f);
        dampingChanged |= ImGui::DragFloat("Angular Damping", &rbc.angularDamping, 0.01f, 0.0f, 10.0f);
        if (dampingChanged) rbc.UpdateDamping();

        if (auto* dyn = rbc.GetDynamic())
        {
            ImGui::SeparatorText("Dynamics");

            float v[3], w[3];
            auto lv = dyn->getLinearVelocity();
            auto av = dyn->getAngularVelocity();
            v[0] = lv.x; v[1] = lv.y; v[2] = lv.z;
            w[0] = av.x; w[1] = av.y; w[2] = av.z;

            if (ImGui::DragFloat3("Linear Velocity", v, 0.05f))
                dyn->setLinearVelocity({ v[0], v[1], v[2] });
            if (ImGui::DragFloat3("Angular Velocity", w, 0.05f))
                dyn->setAngularVelocity({ w[0], w[1], w[2] });

            float mass = dyn->getMass();
            ImGui::Text("Mass: %.3f kg", mass);
            HelpMarker("La masse effective dépend du collider (densité / volume) et peut être recalculée par les colliders.");

            if (ImGui::Button("Wake Up"))
                dyn->wakeUp();
            ImGui::SameLine();
            if (ImGui::Button("Put To Sleep"))
                dyn->putToSleep();
        }

        ImGui::TreePop();
    }
}