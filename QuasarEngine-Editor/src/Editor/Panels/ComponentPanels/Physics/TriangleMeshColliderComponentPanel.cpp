#include "TriangleMeshColliderComponentPanel.h"

#include <imgui/imgui.h>
#include <cfloat>
#include <algorithm>
#include <glm/glm.hpp>

#include <QuasarEngine/Entity/Entity.h>
#include <QuasarEngine/Entity/Components/TransformComponent.h>
#include <QuasarEngine/Entity/Components/Physics/TriangleMeshColliderComponent.h>
#include <QuasarEngine/Entity/Components/Physics/RigidBodyComponent.h>

namespace QuasarEngine
{
    static inline glm::vec3 vmin(const glm::vec3& a, const glm::vec3& b) { return { std::min(a.x,b.x), std::min(a.y,b.y), std::min(a.z,b.z) }; }
    static inline glm::vec3 vmax(const glm::vec3& a, const glm::vec3& b) { return { std::max(a.x,b.x), std::max(a.y,b.y), std::max(a.z,b.z) }; }

    void TriangleMeshColliderComponentPanel::Render(Entity entity)
    {
        if (!entity.HasComponent<TriangleMeshColliderComponent>()) return;
        auto& cc = entity.GetComponent<TriangleMeshColliderComponent>();

        if (!ImGui::TreeNodeEx("Triangle Mesh Collider", ImGuiTreeNodeFlags_DefaultOpen, "Triangle Mesh Collider"))
            return;

        if (ImGui::BeginPopupContextItem())
        {
            if (ImGui::MenuItem("Delete Component"))
                entity.RemoveComponent<TriangleMeshColliderComponent>();
            ImGui::EndPopup();
        }

        if (entity.HasComponent<RigidBodyComponent>())
        {
            auto& rbc = entity.GetComponent<RigidBodyComponent>();
            if (rbc.GetDynamic())
            {
                ImGui::PushStyleColor(ImGuiCol_Text, IM_COL32(255, 120, 120, 255));
                ImGui::TextUnformatted("TriangleMesh requires a Static actor (not Dynamic).");
                ImGui::PopStyleColor();
                if (ImGui::Button("Convert RigidBody to STATIC"))
                {
                    rbc.bodyTypeString = "STATIC";
                    rbc.UpdateBodyType();
                }
            }
        }

        const auto& verts = cc.GetVertices();
        const auto& indices = cc.GetIndices();
        const uint32_t triCount = static_cast<uint32_t>(indices.size() / 3);

        ImGui::SeparatorText("Mesh Info");
        ImGui::Text("Vertices:  %u", (unsigned)verts.size());
        ImGui::Text("Triangles: %u", (unsigned)triCount);

        if (!verts.empty())
        {
            glm::vec3 bbmin(FLT_MAX), bbmax(-FLT_MAX);
            for (const auto& v : verts) { bbmin = vmin(bbmin, v); bbmax = vmax(bbmax, v); }
            const glm::vec3 size = bbmax - bbmin;
            ImGui::Text("Bounds Min: (%.3f, %.3f, %.3f)", bbmin.x, bbmin.y, bbmin.z);
            ImGui::Text("Bounds Max: (%.3f, %.3f, %.3f)", bbmax.x, bbmax.y, bbmax.z);
            ImGui::Text("Bounds Size: (%.3f, %.3f, %.3f)", size.x, size.y, size.z);
        }

        if (!indices.empty())
        {
            const bool fits16 = indices.size() < (1u << 16);
            ImGui::Text("Index Format: %s", fits16 ? "16-bit" : "32-bit");
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

        ImGui::SeparatorText("Material");
        bool matChanged = false;
        matChanged |= ImGui::DragFloat("Friction", &cc.friction, 0.01f, 0.0f, 10.0f);
        matChanged |= ImGui::DragFloat("Bounciness", &cc.bounciness, 0.01f, 0.0f, 1.0f);
        if (matChanged)
            cc.UpdateColliderMaterial();

        ImGui::BeginDisabled(true);
        ImGui::DragFloat("Mass (kg) [static only]", &cc.mass, 0.0f);
        ImGui::EndDisabled();

        if (ImGui::Button("Rebuild Collider"))
            cc.UpdateColliderSize();

        ImGui::SameLine();
        if (ImGui::Button("Reapply Material"))
            cc.UpdateColliderMaterial();

        ImGui::TreePop();
    }
}