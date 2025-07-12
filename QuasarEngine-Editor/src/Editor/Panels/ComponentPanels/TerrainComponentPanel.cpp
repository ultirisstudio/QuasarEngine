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
		if (entity.HasComponent<TerrainComponent>())
		{
			auto& tc = entity.GetComponent<TerrainComponent>();

			if (ImGui::TreeNodeEx("Terrain", ImGuiTreeNodeFlags_DefaultOpen, "Terrain"))
			{
				if (ImGui::BeginPopupContextItem())
				{
					if (ImGui::MenuItem("Delete Component"))
					{
						entity.RemoveComponent<TerrainComponent>();
					}
					ImGui::EndPopup();
				}

				ImGui::Text("Heightmap texture: ");

				std::weak_ptr<Texture2D> texture;

				if (tc.GetHeightMapPath().empty())
				{
					texture = Renderer::m_SceneData.m_AssetManager->getAsset<Texture2D>("Assets/Icons/no_texture.png");
				}
				else
				{
					if (Renderer::m_SceneData.m_AssetManager->isAssetLoaded(tc.GetHeightMapPath()))
					{
						texture = Renderer::m_SceneData.m_AssetManager->getAsset<Texture2D>(tc.GetHeightMapPath());
					}
					else
					{
						AssetToLoad tcAsset;
						tcAsset.id = tc.GetHeightMapPath();
						tcAsset.type = AssetType::TEXTURE;

						Renderer::m_SceneData.m_AssetManager->loadAsset(tcAsset);
					}
				}

				if (texture.lock())
				{
					ImGui::ImageButton("##height_map_texture", (ImTextureID)texture.lock()->GetHandle(), {64.0f, 64.0f}, {0, 1}, {1, 0});
					if (ImGui::BeginDragDropTarget())
					{
						if (const ImGuiPayload* payload = ImGui::AcceptDragDropPayload("CONTENT_BROWSER_ITEM"))
						{
							const wchar_t* path = (const wchar_t*)payload->Data;
							std::filesystem::path filesys = path;
							std::string file = filesys.string();
							tc.SetHeightMap(file);
						}
						ImGui::EndDragDropTarget();
					}
				}

				ImGui::Separator();

				ImGui::Checkbox("Wireframe", &tc.m_PolygonMode);

				if (ImGui::Button("Generate terrain"))
				{
					tc.GenerateTerrain();
				}

				ImGui::SliderInt("Resolution", &tc.rez, 1, 100);
				ImGui::SliderInt("Texture scale", &tc.textureScale, 1, 100);
				ImGui::SliderFloat("Height multiply", &tc.heightMult, 16.0f, 512.0f);

				ImGui::TreePop();
			}
		}
	}
}
