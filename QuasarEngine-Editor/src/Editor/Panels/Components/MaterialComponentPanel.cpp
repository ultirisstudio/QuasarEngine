#include "MaterialComponentPanel.h"

#include <imgui/imgui.h>
#include <filesystem>
#include <glm/gtc/type_ptr.hpp>

#include <QuasarEngine/Renderer/Renderer.h>
#include <QuasarEngine/Entity/Entity.h>
#include <QuasarEngine/Entity/Components/MaterialComponent.h>
#include <QuasarEngine/Resources/Texture2D.h>

#include <QuasarEngine/Scene/Importer/TextureConfigImporter.h>

namespace QuasarEngine
{
    static std::string ToProjectId(const std::filesystem::path& pAbsOrRel)
    {
        std::filesystem::path p = pAbsOrRel;
        if (p.is_absolute())
        {
            const auto root = AssetManager::Instance().getAssetPath();
            std::error_code ec;
            auto rel = std::filesystem::relative(p, root, ec);
            if (!ec && !rel.empty() && rel.native()[0] != '.')
                p = rel;
        }
        
        std::string s = p.generic_string();
        if (s.rfind("Assets/", 0) != 0 && s.rfind("Assets\\", 0) != 0)
            s = "Assets/" + s;
        return s;
    }

    static std::string StripAssetsPrefix(std::string s)
    {
        if (s.rfind("Assets/", 0) == 0)  return s.substr(7);
        if (s.rfind("Assets\\", 0) == 0) return s.substr(7);
        return s;
    }

    MaterialComponentPanel::MaterialComponentPanel()
    {
        const std::string noTexId = "no_texture.png";
        const std::string noTexRel = "Assets/Icons/no_texture.png";

        AssetToLoad asset;
        asset.type = AssetType::TEXTURE;
        asset.id = noTexId;
        asset.path = noTexRel;
        AssetManager::Instance().loadAsset(asset);
    }

    void MaterialComponentPanel::Render(Entity entity)
    {
        Texture2D* noTexture = AssetManager::Instance()
            .getAsset<Texture2D>("no_texture.png").get();

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
                void* handle = (hasTexture && tex) ? reinterpret_cast<void*>(static_cast<std::uintptr_t>(tex->GetHandle()))
                    : (noTexture ? reinterpret_cast<void*>(static_cast<std::uintptr_t>(noTexture->GetHandle())) : nullptr);

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
                        if (const ImGuiPayload* payload = ImGui::AcceptDragDropPayload("CONTENT_BROWSER_ITEM"))
                        {
                            const wchar_t* wpath = (const wchar_t*)payload->Data;
                            std::filesystem::path dropped(wpath);

                            std::string texId = ToProjectId(dropped);
                            auto full = AssetManager::Instance().ResolvePath(texId);

                            mat.SetTexture(s.slot, texId);

                            if (!AssetManager::Instance().isAssetLoaded(texId))
                            {
                                TextureSpecification ts = TextureConfigImporter::ImportTextureConfig(full.generic_string());

                                AssetToLoad a{};
                                a.id = texId;
                                a.path = full.generic_string();
                                a.type = AssetType::TEXTURE;
                                a.spec = ts;

                                AssetManager::Instance().loadAsset(a);
                            }
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
                    {
                        ImGui::Columns(1);
                        ImGui::EndGroup();
                        ImGui::Separator();
                        continue;
                    }

                    const std::string& idProj = pathOpt.value();
                    std::filesystem::path fname = std::filesystem::path(idProj).filename();
                    std::string file = fname.string();
                    size_t lastDot = file.find_last_of('.');
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