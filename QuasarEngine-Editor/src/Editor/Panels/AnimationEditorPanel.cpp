#include "AnimationEditorPanel.h"

namespace QuasarEngine {

    AnimationEditorPanel::AnimationEditorPanel() {
        AnimationTrack track;
        track.name = "Position";
        tracks.push_back(track);
    }

    void AnimationEditorPanel::OnImGuiRender() {
        ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(12, 14));
        ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding, 8.0f);
        ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(9, 8));
        ImGui::PushStyleColor(ImGuiCol_ChildBg, backgroundColor);

        ImGui::Begin("Animation Editor", nullptr, ImGuiWindowFlags_NoScrollbar);

        DrawMiniToolbar();
        DrawTimeline();
        DrawTracks();
        HandleShortcuts();

        ImGui::End();

        ImGui::PopStyleColor();
        ImGui::PopStyleVar(3);
    }

    void AnimationEditorPanel::DrawMiniToolbar() {
        if (ImGui::Button(isPlaying ? " Pause" : " Play")) isPlaying = !isPlaying;
        ImGui::SameLine();
        if (ImGui::Button(" Stop")) { isPlaying = false; currentTime = 0.0f; }
        ImGui::SameLine();
        if (ImGui::Button(" Track")) {
            AnimationTrack t;
            t.name = "Track " + std::to_string(tracks.size() + 1);
            t.color = ImVec4(0.5f + 0.2f * tracks.size(), 0.35f, 1.0f - 0.13f * tracks.size(), 1.0f);
            tracks.push_back(t);
        }
        ImGui::SameLine(); ImGui::Text("|");
        ImGui::SameLine();
        ImGui::Text("Zoom");
        ImGui::SameLine();
        ImGui::SetNextItemWidth(80.0f);
        ImGui::SliderFloat("##Zoom", &timeScale, 0.2f, 3.0f, "%.2fx");
        ImGui::SameLine();
        ImGui::Text("Time: %.2fs", currentTime);
        ImGui::Separator();
    }

    void AnimationEditorPanel::DrawTimeline() {
        ImVec2 start = ImGui::GetCursorScreenPos();
        ImVec2 end = ImVec2(start.x + ImGui::GetContentRegionAvail().x, start.y + timelineHeight);
        ImDrawList* drawList = ImGui::GetWindowDrawList();

        drawList->AddRectFilled(start, end, timelineColor, 0.0f);

        float visibleSeconds = (end.x - start.x) / (pixelsPerSecond * timeScale);
        float tStart = timeOffset;
        float tEnd = timeOffset + visibleSeconds;
        for (float t = std::ceil(tStart); t < tEnd; t += 1.0f) {
            float x = start.x + (t - timeOffset) * pixelsPerSecond * timeScale;
            drawList->AddLine(ImVec2(x, start.y), ImVec2(x, end.y), IM_COL32(180, 180, 180, 180), 1.2f);
            char label[16]; snprintf(label, 16, "%.0f", t);
            drawList->AddText(ImVec2(x + 3, start.y + 6), IM_COL32(50, 50, 50, 255), label);
        }

        float cx = start.x + (currentTime - timeOffset) * pixelsPerSecond * timeScale;
        drawList->AddLine(ImVec2(cx, start.y), ImVec2(cx, end.y), IM_COL32(200, 40, 40, 230), 2.5f);

        ImGui::Dummy(ImVec2(ImGui::GetContentRegionAvail().x, timelineHeight));

        if (ImGui::IsWindowHovered()) {
            float wheel = ImGui::GetIO().MouseWheel;
            if (wheel != 0.0f) timeScale = std::max(0.1f, timeScale + wheel * 0.08f);
            if (ImGui::IsMouseDragging(ImGuiMouseButton_Middle)) timeOffset -= ImGui::GetIO().MouseDelta.x / (pixelsPerSecond * timeScale);
        }
    }

    void AnimationEditorPanel::DrawTracks() {
        float y = ImGui::GetCursorScreenPos().y;
        int idx = 0;
        for (auto& track : tracks) {
            DrawTrack(track, idx, y);
            DrawGraphEditor(track, y + trackHeight + 4.0f);
            y += trackHeight + graphHeight + trackSpacing * 2;
            ++idx;
        }
        ImGui::Dummy(ImVec2(1, 10));
    }

    void AnimationEditorPanel::DrawTrack(AnimationTrack& track, int index, float y) {
        ImVec2 start = ImVec2(ImGui::GetCursorScreenPos().x, y);
        ImVec2 end = ImVec2(start.x + ImGui::GetContentRegionAvail().x, start.y + trackHeight);

        ImDrawList* drawList = ImGui::GetWindowDrawList();
        drawList->AddRectFilled(start, end, IM_COL32(255, 255, 255, 240), 10.0f);
        drawList->AddRect(start, end, IM_COL32(180, 200, 230, 110), 10.0f, 0, 2.0f);
        drawList->AddText(ImVec2(start.x + 7, start.y + 11), IM_COL32(50, 60, 80, 255), track.name.c_str());

        ImVec2 keyframeStart = ImVec2(start.x + labelWidth, start.y);
        float areaWidth = end.x - keyframeStart.x;
        float midY = keyframeStart.y + trackHeight * 0.5f;

        hoveredKeyframe = nullptr;
        static Keyframe* contextKeyframe = nullptr;
        static int contextKeyframeIndex = -1;

        for (size_t i = 0; i < track.keyframes.size(); ++i) {
            Keyframe& key = track.keyframes[i];
            float x = keyframeStart.x + (key.time - timeOffset) * pixelsPerSecond * timeScale;
            ImVec2 pos(x, midY);

            ImU32 col = key.selected ? keyframeSelectedColor : ImGui::ColorConvertFloat4ToU32(track.color);
            drawList->AddCircleFilled(pos, 7.0f, col);
            drawList->AddCircle(pos, 7.0f, keyframeBorder, 2.0f);

            ImGui::SetCursorScreenPos(ImVec2(pos.x - 8, pos.y - 8));
            char btnId[64];
            snprintf(btnId, 64, "##kfbtn_%p_%zu", &track, i);
            ImGui::InvisibleButton(btnId, ImVec2(16, 16));

            if (ImGui::IsItemHovered()) {
                hoveredKeyframe = &key;
                if (ImGui::IsMouseClicked(ImGuiMouseButton_Left)) {
                    if (!key.selected) DeselectAllKeyframes();
                    key.selected = true;
                    draggedKeyframe = &key;
                    dragOffset = ImVec2(ImGui::GetIO().MousePos.x - pos.x, 0);
                }
                if (ImGui::IsMouseClicked(ImGuiMouseButton_Right)) {
                    contextKeyframe = &key;
                    contextKeyframeIndex = (int)i;
                    ImGui::OpenPopup(btnId);
                }
            }
            if (draggedKeyframe == &key && ImGui::IsItemActive() && ImGui::IsMouseDragging(ImGuiMouseButton_Left)) {
                float newX = ImGui::GetIO().MousePos.x - dragOffset.x;
                float newTime = (newX - keyframeStart.x) / (pixelsPerSecond * timeScale) + timeOffset;
                key.time = std::max(0.0f, newTime);
            }
            if (draggedKeyframe == &key && !ImGui::IsMouseDown(ImGuiMouseButton_Left)) {
                draggedKeyframe = nullptr;
            }

            if (ImGui::BeginPopup(btnId)) {
                if (contextKeyframe) {
                    ImGui::InputFloat("Time", &contextKeyframe->time, 0.01f, 0.1f, "%.2f");
                    ImGui::InputFloat("Value", &contextKeyframe->value, 0.01f, 0.1f, "%.2f");
                    ImGui::Combo("Interp.", (int*)&contextKeyframe->interpolation, "Linear\0Bezier\0Hermite\0");
                    if (contextKeyframe->interpolation != InterpolationType::Linear) {
                        ImGui::InputFloat("In Tan.", &contextKeyframe->inTangent, 0.1f, 1.0f);
                        ImGui::InputFloat("Out Tan.", &contextKeyframe->outTangent, 0.1f, 1.0f);
                    }
                    if (ImGui::Button("Supprimer")) {
                        track.keyframes.erase(track.keyframes.begin() + contextKeyframeIndex);
                        ImGui::CloseCurrentPopup();
                        contextKeyframe = nullptr;
                    }
                }
                if (ImGui::Button("OK")) {
                    ImGui::CloseCurrentPopup();
                    contextKeyframe = nullptr;
                }
                ImGui::EndPopup();
            }
        }

        ImVec2 trackAreaMin = keyframeStart;
        ImVec2 trackAreaMax = end;
        ImGui::SetCursorScreenPos(trackAreaMin);
        ImGui::InvisibleButton("##trackadd", ImVec2(trackAreaMax.x - trackAreaMin.x, trackHeight));
        if (ImGui::IsItemHovered() && ImGui::IsMouseClicked(ImGuiMouseButton_Left) && hoveredKeyframe == nullptr) {
            float mx = ImGui::GetIO().MousePos.x;
            float t = (mx - keyframeStart.x) / (pixelsPerSecond * timeScale) + timeOffset;
            AddKeyframe(track, std::clamp(t, 0.0f, timelineLength), 1.0f);
        }
    }

    void AnimationEditorPanel::DrawGraphEditor(AnimationTrack& track, float y) {
        ImVec2 start(ImGui::GetCursorScreenPos().x + labelWidth, y);
        ImVec2 size(ImGui::GetContentRegionAvail().x - labelWidth, graphHeight);

        ImDrawList* drawList = ImGui::GetWindowDrawList();
        drawList->AddRectFilled(start, start + size, IM_COL32(240, 240, 248, 255), 7.0f);

        if (track.keyframes.size() > 1) {
            auto hermite = [](float t, float p0, float p1, float m0, float m1, float dt) {
                float t2 = t * t;
                float t3 = t2 * t;
                return (2 * t3 - 3 * t2 + 1) * p0 +
                    (t3 - 2 * t2 + t) * m0 * dt +
                    (-2 * t3 + 3 * t2) * p1 +
                    (t3 - t2) * m1 * dt;
                };

            for (size_t i = 1; i < track.keyframes.size(); ++i) {
                const auto& k0 = track.keyframes[i - 1];
                const auto& k1 = track.keyframes[i];
                float x0 = start.x + (k0.time - timeOffset) * pixelsPerSecond * timeScale;
                float y0 = start.y + size.y - (k0.value * (size.y - 20.0f));
                float x1 = start.x + (k1.time - timeOffset) * pixelsPerSecond * timeScale;
                float y1 = start.y + size.y - (k1.value * (size.y - 20.0f));
                ImU32 col = track.keyframes[i].selected ? keyframeSelectedColor : ImGui::ColorConvertFloat4ToU32(track.color);

                if (k0.interpolation == InterpolationType::Linear) {
                    drawList->AddLine(ImVec2(x0, y0), ImVec2(x1, y1), col, 2.2f);
                }
                else if (k0.interpolation == InterpolationType::Bezier) {
                    float dt = (k1.time - k0.time);
                    float cx0 = x0 + (dt / 3.0f) * pixelsPerSecond * timeScale; // 1/3
                    float cy0 = y0 - k0.outTangent * (size.y - 20.0f) * dt / 3.0f;

                    float cx1 = x1 - (dt / 3.0f) * pixelsPerSecond * timeScale; // 1/3
                    float cy1 = y1 + k1.inTangent * (size.y - 20.0f) * dt / 3.0f;

                    drawList->AddBezierCubic(
                        ImVec2(x0, y0),
                        ImVec2(cx0, cy0),
                        ImVec2(cx1, cy1),
                        ImVec2(x1, y1),
                        col, 2.2f, 32
                    );
                }
                else if (k0.interpolation == InterpolationType::Hermite) {
                    int steps = 32;
                    float dt = (k1.time - k0.time);
                    for (int j = 0; j < steps; ++j) {
                        float t0 = (float)j / steps;
                        float t1 = (float)(j + 1) / steps;

                        float v0 = hermite(t0, k0.value, k1.value, k0.outTangent, k1.inTangent, dt);
                        float v1 = hermite(t1, k0.value, k1.value, k0.outTangent, k1.inTangent, dt);

                        float px0 = x0 + (x1 - x0) * t0;
                        float px1 = x0 + (x1 - x0) * t1;

                        float py0 = start.y + size.y - (v0 * (size.y - 20.0f));
                        float py1 = start.y + size.y - (v1 * (size.y - 20.0f));

                        drawList->AddLine(ImVec2(px0, py0), ImVec2(px1, py1), col, 2.2f);
                    }
                }
            }
        }

        ImGui::Dummy(size);
    }

    void AnimationEditorPanel::AddKeyframe(AnimationTrack& track, float time, float value) {
        track.keyframes.push_back({ time, value, InterpolationType::Linear, 0, 0, false });
        std::sort(track.keyframes.begin(), track.keyframes.end(), [](const Keyframe& a, const Keyframe& b) { return a.time < b.time; });
    }

    void AnimationEditorPanel::DeselectAllKeyframes() {
        for (auto& tr : tracks)
            for (auto& k : tr.keyframes)
                k.selected = false;
    }

    void AnimationEditorPanel::HandleShortcuts() {
        if (ImGui::IsWindowFocused(ImGuiFocusedFlags_ChildWindows) &&
            ImGui::IsKeyPressed(ImGuiKey_Delete)) {
            RemoveSelectedKeyframes();
        }
    }
    void AnimationEditorPanel::RemoveSelectedKeyframes() {
        for (auto& tr : tracks)
            tr.keyframes.erase(
                std::remove_if(tr.keyframes.begin(), tr.keyframes.end(),
                    [](const Keyframe& k) { return k.selected; }),
                tr.keyframes.end());
    }

    void AnimationEditorPanel::Update(double dt) {
        if (isPlaying) {
            currentTime += static_cast<float>(dt);
            if (currentTime > timelineLength) currentTime = 0.0f;
        }
    }
}
