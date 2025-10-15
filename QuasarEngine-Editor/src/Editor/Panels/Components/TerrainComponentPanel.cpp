#include "TerrainComponentPanel.h"

#include <imgui/imgui.h>
#include <filesystem>

#include <QuasarEngine/Renderer/Renderer.h>
#include <QuasarEngine/Entity/Entity.h>
#include <QuasarEngine/Entity/Components/TerrainComponent.h>

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
        if (!entity.HasComponent<TerrainComponent>()) return;
        auto& tc = entity.GetComponent<TerrainComponent>();

        if (ImGui::TreeNodeEx("Terrain", ImGuiTreeNodeFlags_DefaultOpen, "Terrain"))
        {
            if (ImGui::BeginPopupContextItem())
            {
                if (ImGui::MenuItem("Delete Component"))
                    entity.RemoveComponent<TerrainComponent>();
                ImGui::EndPopup();
            }

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

            bool meshChanged = false;
            bool uniformsChanged = false;

            if (auto tex = texture.lock())
            {
                ImGui::BeginGroup();
                ImGui::ImageButton("##height_map_texture",
                    (ImTextureID)tex->GetHandle(), { 64.0f, 64.0f }, { 0,1 }, { 1,0 });

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

            if (ImGui::SliderInt("Resolution", &tc.rez, 1, 1024)) meshChanged = true;
            if (ImGui::SliderInt("Texture scale", &tc.textureScale, 1, 100)) uniformsChanged = true;
            if (ImGui::SliderFloat("Height multiply", &tc.heightMult, 0.0f, 1024.0f)) meshChanged = true;
            tc.rez = std::max(1, tc.rez);
            tc.textureScale = std::max(1, tc.textureScale);
            tc.heightMult = std::max(0.0f, tc.heightMult);

            {
                const int vxCountX = tc.rez + 1;
                const int vxCountZ = tc.rez + 1;
                const size_t estVerts = size_t(vxCountX) * size_t(vxCountZ);
                const size_t estIdx = size_t(tc.rez) * size_t(tc.rez) * 4;
                ImGui::TextDisabled("Estimated mesh: %zu verts, %zu indices", estVerts, estIdx);
                if (tc.IsGenerated())
                    ImGui::TextColored(ImVec4(0.4f, 1.0f, 0.4f, 1.0f), "Status: Generated");
                else
                    ImGui::TextColored(ImVec4(1.0f, 0.6f, 0.2f, 1.0f), "Status: Not generated");
            }

            {
                bool canGenerate = !tc.GetHeightMapPath().empty();
                if (!canGenerate) ImGui::BeginDisabled();
                if (ImGui::Button(tc.IsGenerated() ? "Regenerate terrain" : "Generate terrain"))
                    tc.GenerateTerrain();
                if (!canGenerate) ImGui::EndDisabled();
            }

            if (s_autoRegen && meshChanged && !tc.GetHeightMapPath().empty())
                tc.GenerateTerrain();

            ImGui::TreePop();
        }
    }
}
