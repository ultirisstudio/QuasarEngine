#include "qepch.h"
#include "TerrainComponentPanel.h"

#include <imgui/imgui.h>
#include <filesystem>

#include <QuasarEngine/Renderer/Renderer.h>
#include <QuasarEngine/Entity/Entity.h>
#include <QuasarEngine/Entity/Components/TerrainComponent.h>
#include <QuasarEngine/Resources/Texture.h>
#include <QuasarEngine/Asset/AssetManager.h>

namespace QuasarEngine
{
    TerrainComponentPanel::TerrainComponentPanel()
    {
    }

    void TerrainComponentPanel::Update()
    {
    }

    void TerrainComponentPanel::Render(Entity entity)
    {
        if (!entity.HasComponent<TerrainComponent>())
            return;

        auto& tc = entity.GetComponent<TerrainComponent>();

        if (ImGui::TreeNodeEx("Terrain", ImGuiTreeNodeFlags_DefaultOpen, "Terrain"))
        {
            if (ImGui::BeginPopupContextItem())
            {
                if (ImGui::MenuItem("Delete Component"))
                    entity.RemoveComponent<TerrainComponent>();
                ImGui::EndPopup();
            }

            bool meshChanged = false;
            bool uniformsChanged = false;
            bool lodChanged = false;

            ImGui::Text("Heightmap:");
            std::weak_ptr<Texture2D> texture;

            if (tc.GetHeightMapId().empty())
            {
                texture = AssetManager::Instance().getAsset<Texture2D>("no_texture.png");
            }
            else
            {
                if (AssetManager::Instance().isAssetLoaded(tc.GetHeightMapId()))
                {
                    texture = AssetManager::Instance().getAsset<Texture2D>(tc.GetHeightMapId());
                }
                else
                {
                    AssetToLoad asset;
                    asset.id = tc.GetHeightMapId();
                    asset.path = tc.GetHeightMapPath();
                    asset.type = AssetType::TEXTURE;
                    AssetManager::Instance().loadAsset(asset);
                }
            }

            if (auto tex = texture.lock())
            {
                ImGui::BeginGroup();

                ImGui::ImageButton(
                    "##height_map_texture",
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

                        tc.SetHeightMap(id, abs.generic_string());
                        meshChanged = true;
                    }
                    ImGui::EndDragDropTarget();
                }

                if (ImGui::IsItemHovered() && !tc.GetHeightMapId().empty())
                {
                    ImGui::BeginTooltip();
                    ImGui::TextUnformatted(tc.GetHeightMapId().c_str());
                    if (!tc.GetHeightMapPath().empty())
                        ImGui::TextDisabled("%s", tc.GetHeightMapPath().c_str());
                    ImGui::Separator();
                    const auto& spec = tex->GetSpecification();
                    ImGui::Text("Size: %d x %d", spec.width, spec.height);
                    ImGui::EndTooltip();
                }

                ImGui::EndGroup();
                ImGui::SameLine();

                ImGui::BeginGroup();
                if (!tc.GetHeightMapId().empty())
                {
                    ImGui::TextWrapped("%s", tc.GetHeightMapId().c_str());
                    if (!tc.GetHeightMapPath().empty())
                        ImGui::TextDisabled("%s", tc.GetHeightMapPath().c_str());
                    if (ImGui::SmallButton("Clear"))
                    {
                        tc.ClearHeightMap();
                        meshChanged = true;
                    }
                }
                else
                {
                    ImGui::TextDisabled("Drop a heightmap here...");
                }
                ImGui::EndGroup();
            }

            ImGui::Separator();

            static bool s_autoRegen = true;
            ImGui::Checkbox("Auto regenerate", &s_autoRegen);
            ImGui::SameLine();
            ImGui::Checkbox("Wireframe", &tc.m_PolygonMode);

            int rez = tc.rez;
            if (ImGui::SliderInt("Resolution", &rez, 1, 1024))
            {
                tc.rez = std::max(1, rez);
                meshChanged = true;
            }

            int texScale = tc.textureScale;
            if (ImGui::SliderInt("Texture scale", &texScale, 1, 100))
            {
                tc.textureScale = std::max(1, texScale);
                uniformsChanged = true;
            }

            float hMult = tc.heightMult;
            if (ImGui::SliderFloat("Height multiply", &hMult, 0.0f, 1024.0f))
            {
                tc.heightMult = std::max(0.0f, hMult);
                tc.SetHeightScale(tc.heightMult);
                lodChanged = true;
            }

            {
                float sizeX = tc.GetTerrainSizeX();
                float sizeZ = tc.GetTerrainSizeZ();
                if (ImGui::DragFloat2("Terrain size (X,Z)", &sizeX, 1.0f, 1.0f, 100000.0f))
                {
                    tc.SetTerrainSize(sizeX, sizeZ);
                    meshChanged = true;
                }
            }

            {
                const int   vxCountX = tc.rez + 1;
                const int   vxCountZ = tc.rez + 1;
                const size_t estVerts = size_t(vxCountX) * size_t(vxCountZ);
                const size_t estIdx = size_t(tc.rez) * size_t(tc.rez) * 4;
                ImGui::TextDisabled("Estimated mesh: %zu verts, %zu indices", estVerts, estIdx);
                if (tc.IsGenerated())
                    ImGui::TextColored(ImVec4(0.4f, 1.0f, 0.4f, 1.0f), "Status: Generated");
                else
                    ImGui::TextColored(ImVec4(1.0f, 0.6f, 0.2f, 1.0f), "Status: Not generated");
            }

            ImGui::Separator();
            {
                bool useQt = tc.UseQuadtree();
                if (ImGui::Checkbox("Use quadtree LOD", &useQt))
                {
                    tc.SetUseQuadtree(useQt);
                }

                if (useQt)
                {
                    auto& lod = tc.GetLODSettings();

                    int maxDepth = lod.maxDepth;
                    if (ImGui::SliderInt("LOD max depth", &maxDepth, 0, 8))
                    {
                        lod.maxDepth = maxDepth;
                        lodChanged = true;
                    }

                    int leafSize = lod.leafNodeSize;
                    if (ImGui::SliderInt("Leaf node size", &leafSize, 2, 128))
                    {
                        lod.leafNodeSize = leafSize;
                        lodChanged = true;
                    }

                    if (ImGui::TreeNode("LOD distances"))
                    {
                        if (lod.lodDistances.size() < size_t(lod.maxDepth + 1))
                            lod.lodDistances.resize(lod.maxDepth + 1, 1000.0f);

                        for (int i = 0; i <= lod.maxDepth; ++i)
                        {
                            char label[64];
                            snprintf(label, sizeof(label), "Depth %d", i);
                            float d = lod.lodDistances[i];
                            if (ImGui::DragFloat(label, &d, 1.0f, 0.0f, 10000.0f))
                            {
                                lod.lodDistances[i] = std::max(0.0f, d);
                                lodChanged = true;
                            }
                        }
                        ImGui::TreePop();
                    }

                    if (tc.HasQuadtree())
                    {
                        ImGui::TextColored(ImVec4(0.6f, 0.8f, 1.0f, 1.0f), "Quadtree: built");
                    }
                    else
                    {
                        ImGui::TextColored(ImVec4(1.0f, 0.6f, 0.4f, 1.0f), "Quadtree: not built");
                    }
                }
            }

            bool canGenerate = !tc.GetHeightMapPath().empty();
            if (!canGenerate) ImGui::BeginDisabled();
            if (ImGui::Button(tc.IsGenerated() ? "Regenerate terrain" : "Generate terrain"))
                tc.GenerateTerrain();
            if (!canGenerate) ImGui::EndDisabled();

            if (canGenerate)
            {
                if (s_autoRegen && meshChanged)
                {
                    tc.GenerateTerrain();
                }
                else if (lodChanged && tc.IsGenerated() && tc.UseQuadtree())
                {
                    tc.BuildQuadtree();
                }
            }

            ImGui::TreePop();
        }
    }
}