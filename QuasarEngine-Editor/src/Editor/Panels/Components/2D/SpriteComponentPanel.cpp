#include "SpriteComponentPanel.h"

#include <imgui/imgui.h>
#include <imgui/imgui_internal.h>

#include <filesystem>
#include <string>
#include <algorithm>
#include <glm/glm.hpp>

#include <QuasarEngine/Entity/Entity.h>
#include <QuasarEngine/Entity/Components/2D/SpriteComponent.h>
#include <QuasarEngine/Resources/Texture2D.h>
#include <QuasarEngine/Asset/AssetManager.h>
#include <QuasarEngine/Scene/Importer/TextureConfigImporter.h>

namespace QuasarEngine
{
    static void DrawAtlasWithUVOverlay(ImTextureID id,
        const glm::vec4& uvEff,
        float previewSz = 128.0f,
        bool drawGrid = true,
        int  gridX = 8,
        int  gridY = 8)
    {
        const ImVec2 size(previewSz, previewSz);
        const ImVec2 uv0_full(0.0f, 1.0f);
        const ImVec2 uv1_full(1.0f, 0.0f);

        ImVec2 p0 = ImGui::GetCursorScreenPos();
        ImGui::Image(id, size, uv0_full, uv1_full);
        ImVec2 p1{ p0.x + size.x, p0.y + size.y };

        ImDrawList* dl = ImGui::GetWindowDrawList();

        if (drawGrid)
        {
            gridX = std::max(1, gridX);
            gridY = std::max(1, gridY);

            const float dx = (p1.x - p0.x) / static_cast<float>(gridX);
            const float dy = (p1.y - p0.y) / static_cast<float>(gridY);

            const ImU32 colMinor = IM_COL32(255, 255, 255, 40);
            const ImU32 colMajor = IM_COL32(255, 255, 255, 90);
            const float thickMinor = 1.0f;
            const float thickMajor = 1.5f;

            for (int i = 1; i < gridX; ++i)
            {
                float x = p0.x + dx * i;
                bool major = (i % 4) == 0;
                dl->AddLine(ImVec2(x, p0.y), ImVec2(x, p1.y),
                    major ? colMajor : colMinor,
                    major ? thickMajor : thickMinor);
            }

            for (int j = 1; j < gridY; ++j)
            {
                float y = p0.y + dy * j;
                bool major = (j % 4) == 0;
                dl->AddLine(ImVec2(p0.x, y), ImVec2(p1.x, y),
                    major ? colMajor : colMinor,
                    major ? thickMajor : thickMinor);
            }

            dl->AddRect(p0, p1, IM_COL32(255, 255, 255, 120), 0.0f, 0, 1.5f);
        }

        auto lerp = [](float a, float b, float t) { return a + (b - a) * t; };

        float u0 = uvEff.x, v0 = uvEff.y;
        float u1 = uvEff.z, v1 = uvEff.w;

        float x0 = lerp(p0.x, p1.x, u0);
        float x1 = lerp(p0.x, p1.x, u1);
        float y0 = lerp(p0.y, p1.y, 1.0f - v0);
        float y1 = lerp(p0.y, p1.y, 1.0f - v1);

        ImVec2 rmin{ std::min(x0, x1), std::min(y0, y1) };
        ImVec2 rmax{ std::max(x0, x1), std::max(y0, y1) };

        const ImU32 col = IM_COL32(255, 180, 0, 255);
        const ImU32 fill = IM_COL32(255, 180, 0, 48);

        dl->AddRectFilled(rmin, rmax, fill);
        dl->AddRect(rmin, rmax, col, 0.0f, 0, 2.0f);
        dl->AddLine(ImVec2(rmin.x, rmin.y), ImVec2(rmax.x, rmax.y), col, 1.0f);
        dl->AddLine(ImVec2(rmin.x, rmax.y), ImVec2(rmax.x, rmin.y), col, 1.0f);
    }

    static void DrawCroppedUVPreview(ImTextureID id, const glm::vec4& uvEff, float size = 96.0f)
    {
        ImVec2 uv0(uvEff.x, 1.0f - uvEff.y);
        ImVec2 uv1(uvEff.z, 1.0f - uvEff.w);
        ImGui::Image(id, ImVec2(size, size), uv0, uv1);
    }

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

	SpriteComponentPanel::SpriteComponentPanel() : s_ShowGrid(true), s_GridX(8), s_GridY(8)
    {
        const std::string noTexId = "no_texture.png";
        const std::string noTexRel = "Assets/Icons/no_texture.png";

        AssetToLoad asset;
        asset.type = AssetType::TEXTURE;
        asset.id = noTexId;
        asset.path = noTexRel;
        AssetManager::Instance().loadAsset(asset);
    }

    void SpriteComponentPanel::Render(Entity entity)
    {
        if (!entity || !entity.IsValid()) return;
        if (!entity.HasComponent<SpriteComponent>()) return;

        Texture2D* noTexture = AssetManager::Instance()
            .getAsset<Texture2D>("no_texture.png").get();

        auto& sc = entity.GetComponent<SpriteComponent>();
        auto& spec = sc.GetSpecification();

        if (ImGui::TreeNodeEx("Sprite", ImGuiTreeNodeFlags_DefaultOpen))
        {
            if (ImGui::BeginPopupContextItem())
            {
                if (ImGui::MenuItem("Delete Component"))
                    entity.RemoveComponent<SpriteComponent>();
                ImGui::EndPopup();
            }

            ImGui::Columns(2, nullptr, false);
            ImGui::SetColumnOffset(1, 160);

            ImGui::TextUnformatted("Texture:");

            Texture* tex = sc.GetTexture();
            void* handle = nullptr;
            if (tex)        handle = reinterpret_cast<void*>(static_cast<std::uintptr_t>(tex->GetHandle()));
            else if (noTexture) handle = reinterpret_cast<void*>(static_cast<std::uintptr_t>(noTexture->GetHandle()));

            const float thumbnailSize = 80.0f;
            const ImVec2 imgBtnSize(thumbnailSize, thumbnailSize);

            ImGui::PushID("##sprite_tex_preview");
            if (handle)
            {
                ImGui::ImageButton("##sprite_tex", (ImTextureID)handle, imgBtnSize, ImVec2{ 0,1 }, ImVec2{ 1,0 });

                if (ImGui::BeginDragDropTarget())
                {
                    if (const ImGuiPayload* payload = ImGui::AcceptDragDropPayload("CONTENT_BROWSER_ITEM"))
                    {
                        const wchar_t* wpath = static_cast<const wchar_t*>(payload->Data);
                        std::filesystem::path dropped(wpath);

                        std::string texId = ToProjectId(dropped);
                        auto full = AssetManager::Instance().ResolvePath(texId);

                        sc.SetTexture(texId);

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
            }
            ImGui::PopID();

            ImGui::NextColumn();

            if (tex)
            {
                std::string idProj = spec.TextureId.value_or(std::string{});
                std::filesystem::path fname = std::filesystem::path(idProj).filename();
                std::string file = fname.string();
                size_t lastDot = file.find_last_of('.');
                std::string name = (lastDot == std::string::npos) ? file : file.substr(0, lastDot);

                ImGui::TextColored(ImVec4(0.2f, 0.7f, 1.0f, 1.0f), "%s", name.c_str());
                ImGui::SameLine();
                if (ImGui::Button("Remove"))
                    sc.ResetTexture();
            }
            else
            {
                static char buffer[512];
                std::string current = spec.TextureId.value_or(std::string{});
                std::snprintf(buffer, sizeof(buffer), "%s", current.c_str());

                ImGui::SetNextItemWidth(ImGui::GetContentRegionAvail().x - 150.0f);
                ImGui::InputText("##SpriteTextureId", buffer, sizeof(buffer));

                ImGui::SameLine();
                if (ImGui::Button("Load", ImVec2(70, 0)))
                {
                    std::string id = buffer;
                    if (!id.empty())
                    {
                        std::string projId = ToProjectId(id);
                        auto full = AssetManager::Instance().ResolvePath(projId);

                        sc.SetTexture(projId);

                        if (!AssetManager::Instance().isAssetLoaded(projId))
                        {
                            TextureSpecification ts = TextureConfigImporter::ImportTextureConfig(full.generic_string());

                            AssetToLoad a{};
                            a.id = projId;
                            a.path = full.generic_string();
                            a.type = AssetType::TEXTURE;
                            a.spec = ts;

                            AssetManager::Instance().loadAsset(a);
                        }
                    }
                }
                ImGui::SameLine();
                if (ImGui::Button("Reset", ImVec2(70, 0)))
                {
                    sc.ResetTexture();
                }
            }

            ImGui::Columns(1);
            ImGui::Separator();

            ImGui::ColorEdit4("Tint (RGBA)", &spec.Color.x, ImGuiColorEditFlags_DisplayRGB | ImGuiColorEditFlags_Float);

            ImGui::Checkbox("Flip X", &spec.FlipX); ImGui::SameLine();
            ImGui::Checkbox("Flip Y", &spec.FlipY);

            ImGui::SetNextItemWidth(200.0f);
            ImGui::InputInt("Sorting Order", &spec.SortingOrder);

            {
                ImGui::Separator();
                ImGui::TextUnformatted("UV (0..1)");
                ImGui::SetNextItemWidth(200.0f);
                ImGui::DragFloat4("##UV", &spec.UV.x, 0.01f, 0.0f, 1.0f, "%.3f");
            }

            {
                ImGui::SetNextItemWidth(200.0f);
                ImGui::DragFloat2("Tiling", &spec.Tiling.x, 0.01f, -1000.0f, 1000.0f, "%.3f");
                ImGui::SetNextItemWidth(200.0f);
                ImGui::DragFloat2("Offset", &spec.Offset.x, 0.01f, -1000.0f, 1000.0f, "%.3f");
            }

            {
                ImGui::Separator();
                ImGui::TextUnformatted("UV Preview");

                Texture* texForPreview = sc.GetTexture();
                if (!texForPreview)
                {
                    texForPreview = noTexture;
                }

                if (texForPreview)
                {
                    ImTextureID imId = (ImTextureID)reinterpret_cast<void*>(
                        static_cast<std::uintptr_t>(texForPreview->GetHandle()));

                    glm::vec4 uvEff = sc.GetEffectiveUV();

                    ImGui::Checkbox("Afficher la grille", &s_ShowGrid);
                    ImGui::SameLine();
                    ImGui::SetNextItemWidth(70.0f);
                    ImGui::InputInt("Div X", &s_GridX);
                    ImGui::SameLine();
                    ImGui::SetNextItemWidth(70.0f);
                    ImGui::InputInt("Div Y", &s_GridY);

                    s_GridX = std::max(1, s_GridX);
                    s_GridY = std::max(1, s_GridY);

                    ImGui::BeginGroup();
                    ImGui::TextUnformatted("Atlas");
                    DrawAtlasWithUVOverlay(imId, uvEff, 128.0f, s_ShowGrid, s_GridX, s_GridY);
                    ImGui::EndGroup();

                    ImGui::SameLine();
                    ImGui::BeginGroup();
                    ImGui::TextUnformatted("Crop");
                    DrawCroppedUVPreview(imId, uvEff, 96.0f);
                    ImGui::EndGroup();
                }
                else
                {
                    ImGui::TextDisabled("Aucune texture.");
                }
            }

            ImGui::TreePop();
        }
    }
}
