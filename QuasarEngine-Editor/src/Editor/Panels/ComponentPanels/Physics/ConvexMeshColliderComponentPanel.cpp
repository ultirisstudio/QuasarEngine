#include "ConvexMeshColliderComponentPanel.h"

#include <imgui/imgui.h>
#include <glm/glm.hpp>

#include <QuasarEngine/Entity/Entity.h>
#include <QuasarEngine/Entity/Components/TransformComponent.h>
#include <QuasarEngine/Entity/Components/Physics/ConvexMeshColliderComponent.h>
#include <QuasarEngine/Entity/Components/Physics/RigidBodyComponent.h>

namespace QuasarEngine
{
    void ConvexMeshColliderComponentPanel::Render(Entity entity)
    {
        if (!entity.HasComponent<ConvexMeshColliderComponent>()) return;
        auto& cc = entity.GetComponent<ConvexMeshColliderComponent>();

        if (!ImGui::TreeNodeEx("Convex Mesh Collider", ImGuiTreeNodeFlags_DefaultOpen, "Convex Mesh Collider"))
            return;

        if (ImGui::BeginPopupContextItem())
        {
            if (ImGui::MenuItem("Delete Component"))
                entity.RemoveComponent<ConvexMeshColliderComponent>();
            ImGui::EndPopup();
        }

        const auto& srcPoints = cc.GetPoints();
        ImGui::SeparatorText("Source Points");
        ImGui::Text("Count: %u", (unsigned)srcPoints.size());
        if (srcPoints.empty())
        {
            ImGui::TextDisabled("No points set. Call SetPoints(...) before Init().");
        }

        bool sizeChanged = false;
        if (ImGui::Checkbox("Use Entity Scale", &cc.m_UseEntityScale))
            sizeChanged = true;
        if (sizeChanged)
            cc.UpdateColliderSize();

        if (cc.m_UseEntityScale && entity.HasComponent<TransformComponent>())
        {
            const auto& s = entity.GetComponent<TransformComponent>().Scale;
            ImGui::Text("Scale: (%.3f, %.3f, %.3f)", s.x, s.y, s.z);
        }

        if (auto* mesh = cc.GetMesh())
        {
            ImGui::SeparatorText("Cooked Hull");
            ImGui::Text("Vertices: %u", (unsigned)mesh->getNbVertices());
            ImGui::Text("Polygons: %u", (unsigned)mesh->getNbPolygons());
            if (mesh->getNbVertices() > 255)
            {
                ImGui::PushStyleColor(ImGuiCol_Text, IM_COL32(255, 120, 120, 255));
                ImGui::TextUnformatted("Warning: convex meshes are typically limited to 255 vertices.");
                ImGui::PopStyleColor();
            }
        }
        else
        {
            ImGui::SeparatorText("Cooked Hull");
            ImGui::TextDisabled("No cooked convex. Click Rebuild if points are set.");
        }

        ImGui::SeparatorText("Material / Mass");
        bool matChanged = false;
        matChanged |= ImGui::DragFloat("Mass (kg)", &cc.mass, 0.1f, 0.0f, 1e9f);
        matChanged |= ImGui::DragFloat("Friction", &cc.friction, 0.01f, 0.0f, 10.0f);
        matChanged |= ImGui::DragFloat("Bounciness", &cc.bounciness, 0.01f, 0.0f, 1.0f);
        if (matChanged)
            cc.UpdateColliderMaterial();

        if (entity.HasComponent<RigidBodyComponent>())
        {
            auto& rb = entity.GetComponent<RigidBodyComponent>();
            if (auto* dyn = rb.GetDynamic())
            {
                ImGui::Text("Actor Mass (effective): %.3f kg", dyn->getMass());
                if (ImGui::Button("Recompute Mass from Density"))
                    cc.UpdateColliderMaterial();
            }
        }

        if (ImGui::Button("Rebuild Collider"))
            cc.UpdateColliderSize();
        ImGui::SameLine();
        if (ImGui::Button("Reapply Material"))
            cc.UpdateColliderMaterial();

        ImGui::TreePop();
    }
}