#include "qepch.h"
#include "ParticleComponentPanel.h"

#include <imgui/imgui.h>
#include <filesystem>

#include <QuasarEngine/Entity/Entity.h>
#include <QuasarEngine/Entity/Components/Particles/ParticleComponent.h>
#include <QuasarEngine/Resources/Texture2D.h>
#include <QuasarEngine/Asset/AssetManager.h>
#include <QuasarEngine/Renderer/Renderer.h>

namespace QuasarEngine
{
    void ParticleComponentPanel::Render(Entity entity)
    {
        if (!entity.HasComponent<ParticleComponent>())
            return;

        auto& pc = entity.GetComponent<ParticleComponent>();

        if (ImGui::TreeNodeEx("Particles", ImGuiTreeNodeFlags_DefaultOpen, "Particles"))
        {
            if (ImGui::BeginPopupContextItem())
            {
                if (ImGui::MenuItem("Delete Component"))
                    entity.RemoveComponent<ParticleComponent>();
                ImGui::EndPopup();
            }

            bool systemNeedsRebuild = false;

            ImGui::Text("Texture:");
            std::weak_ptr<Texture2D> texture;

            if (pc.GetTextureId().empty())
            {
                texture = AssetManager::Instance().getAsset<Texture2D>("no_texture.png");
            }
            else
            {
                if (AssetManager::Instance().isAssetLoaded(pc.GetTextureId()))
                {
                    texture = AssetManager::Instance().getAsset<Texture2D>(pc.GetTextureId());
                }
                else
                {
                    AssetToLoad asset;
                    asset.id = pc.GetTextureId();
                    asset.path = pc.GetTexturePath();
                    asset.type = AssetType::TEXTURE;
                    AssetManager::Instance().loadAsset(asset);
                }
            }

            if (auto tex = texture.lock())
            {
                ImGui::BeginGroup();

                ImGui::ImageButton(
                    "##particle_texture",
                    (ImTextureID)tex->GetHandle(),
                    { 64.0f, 64.0f },
                    { 0, 1 },
                    { 1, 0 }
                );

                if (ImGui::BeginDragDropTarget())
                {
                    if (const ImGuiPayload* pPath = ImGui::AcceptDragDropPayload("CONTENT_BROWSER_ITEM"))
                    {
                        const wchar_t* wpath = (const wchar_t*)pPath->Data;

                        std::filesystem::path abs = WeakCanonical(wpath);
                        std::string id = BuildAssetIdFromAbs(abs, AssetManager::Instance().getAssetPath());

                        pc.SetTexture(id, abs.generic_string());
                        systemNeedsRebuild = true;
                    }
                    ImGui::EndDragDropTarget();
                }

                if (ImGui::IsItemHovered() && !pc.GetTextureId().empty())
                {
                    ImGui::BeginTooltip();
                    ImGui::TextUnformatted(pc.GetTextureId().c_str());
                    if (!pc.GetTexturePath().empty())
                        ImGui::TextDisabled("%s", pc.GetTexturePath().c_str());
                    ImGui::Separator();
                    const auto& spec = tex->GetSpecification();
                    ImGui::Text("Size: %d x %d", spec.width, spec.height);
                    ImGui::EndTooltip();
                }

                ImGui::EndGroup();
                ImGui::SameLine();

                ImGui::BeginGroup();
                if (!pc.GetTextureId().empty())
                {
                    ImGui::TextWrapped("%s", pc.GetTextureId().c_str());
                    if (!pc.GetTexturePath().empty())
                        ImGui::TextDisabled("%s", pc.GetTexturePath().c_str());
                    if (ImGui::SmallButton("Clear"))
                    {
                        pc.ClearTexture();
                    }
                }
                else
                {
                    ImGui::TextDisabled("Drop a particle texture here...");
                }
                ImGui::EndGroup();
            }

            ImGui::Separator();

            ImGui::Checkbox("Playing", &pc.m_Playing);
            ImGui::SameLine();
            ImGui::Checkbox("Loop", &pc.m_Loop);
            ImGui::SameLine();
            ImGui::Checkbox("Local space", &pc.m_LocalSpace);

            int maxPart = pc.m_MaxParticles;
            if (ImGui::SliderInt("Max particles", &maxPart, 1, 2000))
            {
                pc.m_MaxParticles = std::max(1, maxPart);
                systemNeedsRebuild = true;
            }

            ImGui::DragFloat3("Emitter offset", &pc.m_EmitterOffset.x, 0.01f);

            float spawnInterval = pc.m_SpawnInterval;
            if (ImGui::DragFloat("Spawn interval (s)", &spawnInterval, 0.01f, 0.01f, 10.0f))
            {
                pc.m_SpawnInterval = std::max(0.01f, spawnInterval);
            }

            ImGui::DragFloat("Particle life (s)", &pc.m_ParticleLife, 0.01f, 0.01f, 60.0f);

            ImGui::DragFloat("Start size", &pc.m_StartSize, 0.01f, 0.0f, 100.0f);
            ImGui::DragFloat("End size", &pc.m_EndSize, 0.01f, 0.0f, 100.0f);
            ImGui::DragFloat("End size jitter", &pc.m_EndSizeJitter, 0.01f, 0.0f, 10.0f);

            ImGui::DragFloat3("Base velocity", &pc.m_BaseVelocity.x, 0.01f);
            ImGui::DragFloat("Position spread", &pc.m_PositionSpread, 0.01f, 0.0f, 10.0f);
            ImGui::DragFloat("Velocity spread", &pc.m_VelocitySpread, 0.01f, 0.0f, 10.0f);

            ImGui::ColorEdit4("Color start", &pc.m_ColorStart.x);
            ImGui::ColorEdit4("Color end", &pc.m_ColorEnd.x);

            if (systemNeedsRebuild)
                pc.RebuildSystem();

            ImGui::TreePop();
        }
    }
}