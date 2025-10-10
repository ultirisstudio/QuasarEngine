#include "qepch.h"
#include "AnimationComponentPanel.h"

#include <algorithm>
#include <filesystem>
#include <iomanip>
#include <sstream>

#include <imgui/imgui.h>
#include <imgui/imgui_internal.h>

#include <QuasarEngine/Entity/Entity.h>
#include <QuasarEngine/Entity/Components/Animation/AnimationComponent.h>
#include <QuasarEngine/Resources/Model.h>
#include <QuasarEngine/Asset/AssetManager.h>

namespace QuasarEngine
{
    std::string AnimationComponentPanel::FormatSeconds(double sec)
    {
        if (sec < 0.0) sec = 0.0;
        int isec = static_cast<int>(sec + 0.5);
        int mm = isec / 60;
        int ss = isec % 60;
        std::ostringstream oss;
        oss << mm << ":" << std::setw(2) << std::setfill('0') << ss;
        return oss.str();
    }

    void AnimationComponentPanel::Render(Entity entity)
    {
        if (!entity || !entity.IsValid()) return;
        if (!entity.HasComponent<AnimationComponent>()) return;

        auto& ac = entity.GetComponent<AnimationComponent>();

        if (m_LastEntityID != entity.GetUUID()) {
            m_LastEntityID = entity.GetUUID();
            m_SelectedClip = static_cast<int>(ac.CurrentClipIndex());
            if (m_SelectedClip < 0 && ac.GetClipCount() > 0) m_SelectedClip = 0;
        }

        if (ImGui::TreeNodeEx("Animation", ImGuiTreeNodeFlags_DefaultOpen, "Animation"))
        {
            ImGui::SeparatorText("Source");
            {
                std::string modelId = ac.GetModelAssetId();
                char buf[256];
                std::snprintf(buf, sizeof(buf), "%s", modelId.c_str());
                if (ImGui::InputText("Model Asset ID", buf, sizeof(buf))) {
                    ac.SetModelAssetId(std::string(buf));
                }

                if (ImGui::Button("Reload Clips from Asset")) {
                    m_RequestReload = true;
                }
                if (m_RequestReload) {
                    m_RequestReload = false;
                    if (!ac.GetModelAssetId().empty()) {
                        std::filesystem::path full = AssetManager::Instance().ResolvePath(ac.GetModelAssetId());
                        auto clips = LoadAnimationClips(full.generic_string());
                        ac.SetClips(std::move(clips));
                        m_SelectedClip = ac.GetClipCount() > 0 ? 0 : -1;
                        if (m_SelectedClip >= 0) ac.Play((size_t)m_SelectedClip, ac.GetLoop(), ac.GetSpeed());
                    }
                }
            }

            ImGui::SeparatorText("Info");
            {
                size_t clipCount = ac.GetClipCount();
                int bones = 0;
                if (!ac.GetModelAssetId().empty() && AssetManager::Instance().isAssetLoaded(ac.GetModelAssetId())) {
                    auto model = AssetManager::Instance().getAsset<Model>(ac.GetModelAssetId());
                    if (model) bones = model->GetBoneCount();
                }
                ImGui::Text("Clips: %zu  |  Bones: %d", clipCount, bones);
            }

            ImGui::SeparatorText("Clips");
            {
                const size_t n = ac.GetClipCount();
                if (n == 0) {
                    ImGui::TextDisabled("No clips loaded.");
                }
                else {
                    if (m_SelectedClip < 0 || m_SelectedClip >= (int)n) m_SelectedClip = 0;

                    std::vector<std::string> labels;
                    labels.reserve(n);
                    for (size_t i = 0; i < n; ++i) {
                        const AnimationClip* clip = ac.GetClip(i);
                        double durSec = 0.0;
                        if (clip && clip->ticksPerSecond > 0.0f)
                            durSec = (double)clip->duration / (double)clip->ticksPerSecond;
                        std::ostringstream oss;
                        oss << (clip ? clip->name : std::string("Clip ") + std::to_string(i))
                            << "  (" << FormatSeconds(durSec) << ")";
                        labels.push_back(oss.str());
                    }

                    if (ImGui::BeginCombo("Active Clip", labels[(size_t)m_SelectedClip].c_str())) {
                        for (int i = 0; i < (int)n; ++i) {
                            bool selected = (i == m_SelectedClip);
                            if (ImGui::Selectable(labels[(size_t)i].c_str(), selected)) {
                                m_SelectedClip = i;
                                if (ac.IsPlaying() || ac.IsPaused()) {
                                    ac.Play((size_t)m_SelectedClip, ac.GetLoop(), ac.GetSpeed());
                                }
                            }
                            if (selected) ImGui::SetItemDefaultFocus();
                        }
                        ImGui::EndCombo();
                    }
                }
            }

            ImGui::SeparatorText("Controls");
            {
                bool loop = ac.GetLoop();
                if (ImGui::Checkbox("Loop", &loop)) ac.SetLoop(loop);

                ImGui::SameLine();
                float speed = ac.GetSpeed();
                if (ImGui::DragFloat("Speed", &speed, 0.01f, 0.0f, 4.0f, "%.2f")) {
                    ac.SetSpeed(speed);
                }

                bool inPlace = ac.GetInPlace();
                if (ImGui::Checkbox("In-place (no root motion)", &inPlace)) {
                    ac.SetInPlace(inPlace);
                }

                if (!ac.GetModelAssetId().empty() && AssetManager::Instance().isAssetLoaded(ac.GetModelAssetId())) {
                    auto model = AssetManager::Instance().getAsset<Model>(ac.GetModelAssetId());
                    if (model) {
                        const auto& boneMap = model->GetBoneInfoMap();
                        std::vector<std::string> boneNames;
                        boneNames.reserve(boneMap.size());
                        for (auto& kv : boneMap) boneNames.push_back(kv.first);
                        std::sort(boneNames.begin(), boneNames.end());

                        int currentIndex = 0;
                        if (!boneNames.empty()) {
                            auto it = std::find(boneNames.begin(), boneNames.end(), ac.GetRootBoneName());
                            if (it != boneNames.end()) currentIndex = (int)std::distance(boneNames.begin(), it);
                        }

                        const char* preview = boneNames.empty()
                            ? "(no bones)"
                            : boneNames[(size_t)currentIndex].c_str();

                        ImGui::SetNextItemWidth(240.0f);
                        if (ImGui::BeginCombo("Root bone", preview)) {
                            for (int i = 0; i < (int)boneNames.size(); ++i) {
                                bool selected = (i == currentIndex);
                                if (ImGui::Selectable(boneNames[(size_t)i].c_str(), selected)) {
                                    currentIndex = i;
                                    ac.SetRootBoneName(boneNames[(size_t)i]);
                                }
                                if (selected) ImGui::SetItemDefaultFocus();
                            }
                            ImGui::EndCombo();
                        }
                        ImGui::SameLine();
                        ImGui::TextDisabled("(ex: Hips / Root)");
                    }
                }

                const bool canPlay = (ac.GetClipCount() > 0);
                const bool playing = ac.IsPlaying();
                const bool paused = ac.IsPaused();

                if (ImGui::Button(playing ? "Playing..." : "Play")) {
                    if (canPlay) {
                        size_t idx = (m_SelectedClip >= 0) ? (size_t)m_SelectedClip : 0;
                        ac.Play(idx, ac.GetLoop(), ac.GetSpeed());
                    }
                }
                ImGui::SameLine();
                if (ImGui::Button(paused ? "Paused" : "Pause")) {
                    if (playing) ac.Pause();
                }
                ImGui::SameLine();
                if (ImGui::Button("Stop")) {
                    if (playing || paused) ac.Stop();
                }
            }

            {
                const AnimationClip* cur = ac.GetCurrentClip();
                if (cur) {
                    double durSec = (cur->ticksPerSecond > 0.0f && cur->duration > 0.0f)
                        ? (double)cur->duration / (double)cur->ticksPerSecond
                        : 0.0;

                    if (durSec > 0.0) {
                        double t = ac.GetTimeSeconds();
                        float tf = static_cast<float>(std::fmod(t, durSec));
                        ImGui::SeparatorText("Time");
                        ImGui::Text(" %s / %s", FormatSeconds(tf).c_str(), FormatSeconds(durSec).c_str());
                        if (ImGui::SliderFloat("Scrub", &tf, 0.0f, (float)durSec, "%.2f s")) {
                            ac.SetTimeSeconds(tf);
                        }

                        ImGui::TextDisabled("Clip: \"%s\"  |  duration: %.2f ticks  |  tps: %.2f  |  channels: %zu",
                            cur->name.c_str(), cur->duration, cur->ticksPerSecond, cur->channels.size());
                    }
                }
            }

            ImGui::TreePop();
        }
    }
}