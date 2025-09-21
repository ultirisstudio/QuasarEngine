#include "BoxColliderComponentPanel.h"

#include <imgui/imgui.h>
#include <glm/glm.hpp>
#include <glm/gtc/type_ptr.hpp>

#include <QuasarEngine/Entity/Entity.h>
#include <QuasarEngine/Entity/Components/TransformComponent.h>
#include <QuasarEngine/Entity/Components/Physics/BoxColliderComponent.h>

namespace QuasarEngine
{
    static inline glm::vec3 ClampVec3Min(const glm::vec3& v, float mn)
    {
        return { v.x < mn ? mn : v.x, v.y < mn ? mn : v.y, v.z < mn ? mn : v.z };
    }

    void BoxColliderComponentPanel::Render(Entity entity)
    {
        if (!entity.HasComponent<BoxColliderComponent>()) return;
        auto& cc = entity.GetComponent<BoxColliderComponent>();

        if (!ImGui::TreeNodeEx("Box Collider", ImGuiTreeNodeFlags_DefaultOpen, "Box Collider")) return;

        if (ImGui::BeginPopupContextItem())
        {
            if (ImGui::MenuItem("Delete Component"))
                entity.RemoveComponent<BoxColliderComponent>();
            ImGui::EndPopup();
        }

        bool sizeChanged = false;

        if (ImGui::Checkbox("Use Entity Scale", &cc.m_UseEntityScale))
            sizeChanged = true;

        if (!cc.m_UseEntityScale)
        {
            glm::vec3 sz = cc.m_Size;
            if (ImGui::DragFloat3("Size (XYZ)", glm::value_ptr(sz), 0.01f, 0.001f, 1e6f))
            {
                cc.m_Size = ClampVec3Min(sz, 0.001f);
                sizeChanged = true;
            }
        }

        if (sizeChanged)
            cc.UpdateColliderSize();

        glm::vec3 usedSize = cc.m_UseEntityScale
            ? entity.GetComponent<TransformComponent>().Scale
            : cc.m_Size;

        usedSize = ClampVec3Min(usedSize, 0.001f);
        const glm::vec3 halfExtents = 0.5f * usedSize;
        const float volume = usedSize.x * usedSize.y * usedSize.z;

        ImGui::SeparatorText("Shape");
        ImGui::Text("Half Extents:  (%.3f, %.3f, %.3f)", halfExtents.x, halfExtents.y, halfExtents.z);
        ImGui::Text("Volume:        %.6f m^3", volume);

        ImGui::SeparatorText("Material / Mass");
        bool matChanged = false;
        matChanged |= ImGui::DragFloat("Mass (kg)", &cc.mass, 0.1f, 0.0f, 1e6f);
        matChanged |= ImGui::DragFloat("Friction", &cc.friction, 0.01f, 0.0f, 10.0f);
        matChanged |= ImGui::DragFloat("Bounciness", &cc.bounciness, 0.01f, 0.0f, 1.0f);

        if (matChanged)
            cc.UpdateColliderMaterial();

        if (ImGui::Button("Rebuild Shape"))
            cc.UpdateColliderSize();

        ImGui::TreePop();
    }
}