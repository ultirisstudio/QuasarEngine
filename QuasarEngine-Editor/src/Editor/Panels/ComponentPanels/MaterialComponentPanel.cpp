#include "MaterialComponentPanel.h"

#include <imgui/imgui.h>
#include <filesystem>
#include <glm/gtc/type_ptr.hpp>

#include <QuasarEngine/Renderer/Renderer.h>
#include <QuasarEngine/Entity/Entity.h>
#include <QuasarEngine/Entity/Components/MaterialComponent.h>
#include <QuasarEngine/Resources/Texture2D.h>

namespace QuasarEngine
{
	MaterialComponentPanel::MaterialComponentPanel()
	{
        AssetToLoad asset;
        asset.type = AssetType::TEXTURE;
        asset.id = "Assets/Icons/no_texture.png";
        Renderer::m_SceneData.m_AssetManager->loadAsset(asset);
	}

    void MaterialComponentPanel::Render(Entity entity)
    {
        Texture2D* noTexture = Renderer::m_SceneData.m_AssetManager->getAsset<Texture2D>("Assets/Icons/no_texture.png").get();

        if (!entity.HasComponent<MaterialComponent>())
            return;

        auto& mc = entity.GetComponent<MaterialComponent>();
        Material& mat = mc.GetMaterial();

        struct MaterialSlot {
            const char* label;
            const char* imguiId;
            TextureType slot;
            bool hasColor;
            float* floatValue = nullptr;
            glm::vec4* colorValue = nullptr;
            const char* dragdropId;
        };

        float thumbnailSize = 80.0f;
        static ImVec2 imgBtnSize(thumbnailSize, thumbnailSize);

        std::vector<MaterialSlot> slots = {
            { "Albedo",   "##albedo_texture",    TextureType::Albedo,   true,  nullptr, &mat.GetAlbedo(), "CONTENT_BROWSER_ITEM" },
            { "Normal",   "##normal_texture",    TextureType::Normal,   false, nullptr, nullptr,          "CONTENT_BROWSER_ITEM" },
            { "Metallic", "##metallic_texture",  TextureType::Metallic, false, &mat.GetMetallic(), nullptr, "CONTENT_BROWSER_ITEM" },
            { "Roughness","##roughness_texture", TextureType::Roughness,false, &mat.GetRoughness(), nullptr, "CONTENT_BROWSER_ITEM" },
            { "AO",       "##ao_texture",        TextureType::AO,       false, &mat.GetAO(), nullptr,     "CONTENT_BROWSER_ITEM" }
        };

        if (ImGui::TreeNodeEx("Material", ImGuiTreeNodeFlags_DefaultOpen))
        {
            if (ImGui::BeginPopupContextItem())
            {
                if (ImGui::MenuItem("Delete Component"))
                    entity.RemoveComponent<MaterialComponent>();
                ImGui::EndPopup();
            }

            for (auto& s : slots)
            {
                bool hasTexture = mat.HasTexture(s.slot);
                Texture* tex = mat.GetTexture(s.slot);
                void* handle = (hasTexture && tex) ? tex->GetHandle() : (noTexture ? noTexture->GetHandle() : nullptr);

                ImGui::BeginGroup();
                ImGui::Columns(2, nullptr, false);
                ImGui::SetColumnOffset(1, 160);

                ImGui::Text("%s Texture:", s.label);

                if (handle)
                {
                    ImGui::PushID(s.imguiId);
                    ImGui::ImageButton(s.imguiId, (ImTextureID)handle, imgBtnSize, { 0,1 }, { 1,0 });
                    if (ImGui::BeginDragDropTarget())
                    {
                        if (const ImGuiPayload* payload = ImGui::AcceptDragDropPayload(s.dragdropId))
                        {
                            const wchar_t* path = (const wchar_t*)payload->Data;
                            mat.SetTexture(s.slot, std::filesystem::path(path).generic_string());
                        }
                        ImGui::EndDragDropTarget();
                    }
                    ImGui::PopID();
                }
                ImGui::NextColumn();

                if (hasTexture && tex)
                {
                    auto pathOpt = mat.GetTexturePath(s.slot);
                    if (!pathOpt.has_value())
                        return;

                    const std::string& path = pathOpt.value();
                    size_t slash = path.find_last_of("/\\");
                    std::string file = (slash == std::string::npos) ? path : path.substr(slash + 1);
                    size_t lastDot = file.find_last_of(".");
                    std::string name = (lastDot == std::string::npos) ? file : file.substr(0, lastDot);

                    ImGui::TextColored(ImVec4(0.2f, 0.7f, 1.0f, 1.0f), "%s", name.c_str());
                    ImGui::SameLine();
                    ImGui::PushID(s.imguiId);
                    if (ImGui::Button("Remove"))
                        mat.ResetTexture(s.slot);
                    ImGui::PopID();
                }
                else
                {
                    if (s.hasColor && s.colorValue)
                    {
                        ImGui::Text("Albedo Color:");
                        ImGui::ColorEdit4(("##AlbedoColor" + std::string(s.imguiId)).c_str(), glm::value_ptr(*s.colorValue));
                    }
                    else if (s.floatValue)
                    {
                        ImGui::Text("%s:", s.label);
                        ImGui::SliderFloat(("##" + std::string(s.label)).c_str(), s.floatValue, 0.0f, 1.0f);
                    }
                }

                ImGui::Columns(1);
                ImGui::EndGroup();
                ImGui::Separator();
            }

            ImGui::TreePop();
        }
    }
}
