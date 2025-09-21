#include "HeightfieldColliderComponentPanel.h"

#include <imgui/imgui.h>
#include <glm/glm.hpp>

#include <PxPhysicsAPI.h>

#include <QuasarEngine/Entity/Entity.h>
#include <QuasarEngine/Entity/Components/TransformComponent.h>
#include <QuasarEngine/Entity/Components/Physics/RigidBodyComponent.h>
#include <QuasarEngine/Entity/Components/Physics/HeightfieldColliderComponent.h>

namespace QuasarEngine
{
    static inline float AbsMax3(float a, float b, float c) { a = std::abs(a); b = std::abs(b); c = std::abs(c); return a > b ? (a > c ? a : c) : (b > c ? b : c); }

    void HeightfieldColliderComponentPanel::Render(Entity entity)
    {
        if (!entity.HasComponent<HeightfieldColliderComponent>()) return;
        auto& cc = entity.GetComponent<HeightfieldColliderComponent>();

        if (!ImGui::TreeNodeEx("Heightfield Collider", ImGuiTreeNodeFlags_DefaultOpen, "Heightfield Collider"))
            return;

        if (ImGui::BeginPopupContextItem())
        {
            if (ImGui::MenuItem("Delete Component"))
                entity.RemoveComponent<HeightfieldColliderComponent>();
            ImGui::EndPopup();
        }

        if (entity.HasComponent<RigidBodyComponent>())
        {
            auto& rb = entity.GetComponent<RigidBodyComponent>();
            if (rb.GetDynamic())
            {
                ImGui::PushStyleColor(ImGuiCol_Text, IM_COL32(255, 120, 120, 255));
                ImGui::TextUnformatted("Heightfields doivent être attachés à un acteur Static (pas Dynamic).");
                ImGui::PopStyleColor();
                if (ImGui::Button("Convertir le RigidBody en STATIC"))
                {
                    rb.bodyTypeString = "STATIC";
                    rb.UpdateBodyType();
                }
            }
        }

        bool sizeChanged = false;
        if (ImGui::Checkbox("Use Entity Scale", &cc.m_UseEntityScale))
            sizeChanged = true;

        if (sizeChanged)
            cc.UpdateColliderSize();

        physx::PxShape* shape = cc.GetShape();
        if (shape)
        {
            physx::PxGeometryHolder gh = shape->getGeometry();

            if (gh.getType() == physx::PxGeometryType::eHEIGHTFIELD)
            {
                const physx::PxHeightFieldGeometry& geom = gh.heightField();

                uint32_t rows = 0, cols = 0;
                if (geom.heightField)
                {
                    rows = geom.heightField->getNbRows();
                    cols = geom.heightField->getNbColumns();
                }

                const float heightScale = geom.heightScale;
                const float rowScale = geom.rowScale;
                const float colScale = geom.columnScale;

                const float worldSizeX = (cols > 1 ? (cols - 1) * colScale : 0.0f);
                const float worldSizeZ = (rows > 1 ? (rows - 1) * rowScale : 0.0f);

                ImGui::SeparatorText("Geometry");
                ImGui::Text("Rows x Cols:   %u x %u", (unsigned)rows, (unsigned)cols);
                ImGui::Text("ColumnScale X: %.6f m", colScale);
                ImGui::Text("RowScale    Z: %.6f m", rowScale);
                ImGui::Text("HeightScale Y: %.6f m/unit", heightScale);
                ImGui::Text("World Size:     %.3f m (X) x %.3f m (Z)", worldSizeX, worldSizeZ);

                if (cc.m_UseEntityScale && entity.HasComponent<TransformComponent>())
                {
                    const auto& s = entity.GetComponent<TransformComponent>().Scale;
                    ImGui::Text("Entity Scale:   (%.3f, %.3f, %.3f)", s.x, s.y, s.z);
                }
            }
            else
            {
                ImGui::SeparatorText("Geometry");
                ImGui::TextDisabled("Shape présent mais non heightfield (rebuild conseillé).");
            }
        }
        else
        {
            ImGui::SeparatorText("Geometry");
            ImGui::TextDisabled("Aucun shape attaché (rebuild conseillé).");
        }

        ImGui::SeparatorText("Material");
        bool matChanged = false;
        matChanged |= ImGui::DragFloat("Friction", &cc.friction, 0.01f, 0.0f, 10.0f);
        matChanged |= ImGui::DragFloat("Bounciness", &cc.bounciness, 0.01f, 0.0f, 1.0f);
        if (matChanged)
            cc.UpdateColliderMaterial();

        ImGui::BeginDisabled(true);
        ImGui::DragFloat("Mass (kg) [static]", &cc.mass, 0.0f);
        ImGui::EndDisabled();

        if (ImGui::Button("Rebuild Collider"))
            cc.UpdateColliderSize();

        ImGui::SameLine();
        if (ImGui::Button("Reapply Material"))
            cc.UpdateColliderMaterial();

        ImGui::TreePop();
    }
}