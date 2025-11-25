#include <algorithm>
#include <cmath>
#include "AnimationEditor.h"

namespace QuasarEngine
{
    int CurveScalar::add(float time, float value) {
        KeyframeScalar k;
        k.time = time;
        k.value = value;
        k.interp = Interpolation::Linear;
        k.inMode = TangentMode::Auto;
        k.outMode = TangentMode::Auto;
        k.inTangent = 0.f;
        k.outTangent = 0.f;

        auto it = std::lower_bound(
            keys.begin(), keys.end(), time,
            [](const KeyframeScalar& a, float t) { return a.time < t; });

        it = keys.insert(it, k);
        hot_ = {};
        return static_cast<int>(it - keys.begin());
    }

    void CurveScalar::remove_if_selected() {
        keys.erase(
            std::remove_if(keys.begin(), keys.end(),
                [](const KeyframeScalar& k) { return k.selected; }),
            keys.end());
        hot_ = {};
    }

    float CurveScalar::evaluate(float t) const {
        if (keys.empty()) return 0.f;
        if (keys.size() == 1) return keys[0].value;

        const float firstT = keys.front().time;
        const float lastT = keys.back().time;

        if (t <= firstT) {
            switch (pre) {
            case Extrapolation::Hold:     return keys.front().value;
            case Extrapolation::Linear:   return linear_extrap_left(t);
            case Extrapolation::Loop:     return evaluate(wrap_repeat(t, firstT, lastT));
            case Extrapolation::PingPong: return evaluate(wrap_pingpong(t, firstT, lastT));
            }
        }
        if (t >= lastT) {
            switch (post) {
            case Extrapolation::Hold:     return keys.back().value;
            case Extrapolation::Linear:   return linear_extrap_right(t);
            case Extrapolation::Loop:     return evaluate(wrap_repeat(t, firstT, lastT));
            case Extrapolation::PingPong: return evaluate(wrap_pingpong(t, firstT, lastT));
            }
        }

        const int i = find_segment(t);
        const auto& k0 = keys[(size_t)i];
        const auto& k1 = keys[(size_t)i + 1];
        const float dt = (k1.time - k0.time);
        if (dt == 0.f) return k0.value;
        const float u = (t - k0.time) / dt;

        switch (k0.interp) {
        case Interpolation::Step:
            return k0.value;

        case Interpolation::Linear:
            return k0.value + (k1.value - k0.value) * u;

        case Interpolation::Bezier: {
            const float m0 = k0.outTangent * dt;
            const float m1 = k1.inTangent * dt;
            return cubic_hermite(u, k0.value, k1.value, m0, m1);
        }

        case Interpolation::Hermite: {
            const float m0 = resolve_tangent(k0, i, -1) * dt;
            const float m1 = resolve_tangent(k1, i + 1, +1) * dt;
            return cubic_hermite(u, k0.value, k1.value, m0, m1);
        }

        case Interpolation::CatmullRom: {
            const float p0 = value_at_index_clamped(i - 1);
            const float p1 = k0.value;
            const float p2 = k1.value;
            const float p3 = value_at_index_clamped(i + 2);
            return catmull_rom(p0, p1, p2, p3, u);
        }
        }

        return k0.value;
    }

    float CurveScalar::cubic_hermite(float u, float p0, float p1, float m0, float m1) {
        const float u2 = u * u;
        const float u3 = u2 * u;
        return (2.f * u3 - 3.f * u2 + 1.f) * p0
            + (u3 - 2.f * u2 + u) * m0
            + (-2.f * u3 + 3.f * u2) * p1
            + (u3 - u2) * m1;
    }

    float CurveScalar::catmull_rom(float p0, float p1, float p2, float p3, float u) {
        const float u2 = u * u;
        const float u3 = u2 * u;
        return 0.5f * ((2.f * p1)
            + (-p0 + p2) * u
            + (2.f * p0 - 5.f * p1 + 4.f * p2 - p3) * u2
            + (-p0 + 3.f * p1 - 3.f * p2 + p3) * u3);
    }

    float CurveScalar::value_at_index_clamped(int idx) const {
        if (keys.empty()) return 0.f;
        if (idx < 0) return keys.front().value;
        if (idx >= static_cast<int>(keys.size())) return keys.back().value;
        return keys[(size_t)idx].value;
    }

    int CurveScalar::find_segment(float t) const {
        if (hot_.index >= 0 && t >= hot_.t0 && t <= hot_.t1) return hot_.index;

        int lo = 0;
        int hi = static_cast<int>(keys.size()) - 2;
        while (lo <= hi) {
            const int mid = (lo + hi) / 2;
            const float t0 = keys[(size_t)mid].time;
            const float t1 = keys[(size_t)mid + 1].time;

            if (t < t0) hi = mid - 1;
            else if (t > t1) lo = mid + 1;
            else {
                hot_.index = mid; hot_.t0 = t0; hot_.t1 = t1;
                return mid;
            }
        }

        const int idx = std::clamp(lo, 0, static_cast<int>(keys.size()) - 2);
        hot_.index = idx; hot_.t0 = keys[(size_t)idx].time; hot_.t1 = keys[(size_t)idx + 1].time;
        return idx;
    }

    float CurveScalar::resolve_tangent(const KeyframeScalar& k, int index, int direction) const {
        const bool isIn = (direction < 0);

        switch (isIn ? k.inMode : k.outMode) {
        case TangentMode::Flat:
            return 0.f;

        case TangentMode::Linear: {
            const int i0 = std::clamp(index - 1, 0, static_cast<int>(keys.size()) - 1);
            const int i1 = std::clamp(index + 1, 0, static_cast<int>(keys.size()) - 1);
            const float dt = keys[(size_t)i1].time - keys[(size_t)i0].time;
            if (dt == 0.f) return 0.f;
            return (keys[(size_t)i1].value - keys[(size_t)i0].value) / dt;
        }

        case TangentMode::Free:
            return isIn ? k.inTangent : k.outTangent;

        case TangentMode::AutoClamped:
        case TangentMode::Auto: {
            const int i = index;
            const int il = std::clamp(i - 1, 0, static_cast<int>(keys.size()) - 1);
            const int ir = std::clamp(i + 1, 0, static_cast<int>(keys.size()) - 1);

            const float dt = keys[(size_t)ir].time - keys[(size_t)il].time;
            if (dt == 0.f) return 0.f;

            float slope = (keys[(size_t)ir].value - keys[(size_t)il].value) / dt;

            const float vi = keys[(size_t)i].value;
            const float vl = keys[(size_t)il].value;
            const float vr = keys[(size_t)ir].value;
            const bool isMax = (vi > vl && vi > vr);
            const bool isMin = (vi < vl && vi < vr);
            if (isMax || isMin) slope = 0.f;

            return slope;
        }
        }

        return 0.f;
    }

    float CurveScalar::linear_extrap_left(float t) const {
        if (keys.size() < 2) return keys.empty() ? 0.f : keys.front().value;
        const auto& k0 = keys[0];
        const auto& k1 = keys[1];
        const float dt = (k1.time - k0.time);
        if (dt == 0.f) return k0.value;
        const float dvdt = (k1.value - k0.value) / dt;
        return k0.value + (t - k0.time) * dvdt;
    }

    float CurveScalar::linear_extrap_right(float t) const {
        if (keys.size() < 2) return keys.empty() ? 0.f : keys.back().value;
        const auto& k0 = keys[keys.size() - 2];
        const auto& k1 = keys[keys.size() - 1];
        const float dt = (k1.time - k0.time);
        if (dt == 0.f) return k1.value;
        const float dvdt = (k1.value - k0.value) / dt;
        return k1.value + (t - k1.time) * dvdt;
    }

    AnimationEditor::AnimationEditor(EditorContext& context) : IEditorModule(context) {
        clip_.name = "Default Clip";
        TrackScalar t; t.name = "PositionX"; t.color = ImVec4(0.31f, 0.51f, 1.f, 1.f);
        t.curve.add(0.0f, 0.0f);
        t.curve.add(1.0f, 1.0f);
        t.curve.keys[0].interp = Interpolation::Hermite;
        t.curve.keys[1].interp = Interpolation::Hermite;
        clip_.scalarTracks.push_back(t);
    }

    AnimationEditor::~AnimationEditor()
    {

    }

    void AnimationEditor::Update(double dt) {
        player_.update(dt, clip_);
    }

    void AnimationEditor::RenderUI() {
        const ImGuiStyle& S = ImGui::GetStyle();
        auto C = [&](ImGuiCol col)->ImU32 { return ImGui::GetColorU32(S.Colors[col]); };
        grid_.bg = C(ImGuiCol_WindowBg);
        grid_.timeline = C(ImGuiCol_FrameBg);
        grid_.gridMajor = C(ImGuiCol_Border);
        grid_.gridMinor = C(ImGuiCol_Separator);
        grid_.playhead = C(ImGuiCol_CheckMark);
        grid_.curve = C(ImGuiCol_PlotLines);
        grid_.curveSel = C(ImGuiCol_HeaderHovered);
        grid_.keyFill = C(ImGuiCol_Button);
        grid_.keySel = C(ImGuiCol_Header);
        grid_.keyEdge = C(ImGuiCol_Border);
        grid_.marker = C(ImGuiCol_PlotHistogram);

        ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(12, 14));
        ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding, 8.0f);
        ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(9, 8));
        ImGui::PushStyleColor(ImGuiCol_ChildBg, grid_.bg);

        ImGui::Begin("Animation Editor", nullptr, ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse);

        DrawToolbar();
        DrawTimeline();
        DrawTracks();
        HandleShortcuts();

        ImGui::End();

        ImGui::PopStyleColor();
        ImGui::PopStyleVar(3);
    }

    void AnimationEditor::DrawToolbar() {
        if (ImGui::BeginTable("ToolbarTbl", 3, ImGuiTableFlags_SizingStretchProp)) {
            ImGui::TableNextColumn();
            ImGui::BeginGroup();
            ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(10, 6));
            if (ImGui::Button(player_.playing ? "Pause" : "Play")) player_.playing = !player_.playing;
            ImGui::SameLine();
            if (ImGui::Button("Stop")) { player_.stop(); }
            ImGui::SameLine();
            ImGui::SetNextItemWidth(80);
            ImGui::DragFloat("Speed", &player_.speed, 0.01f, -4.f, 4.f, "%.2fx", ImGuiSliderFlags_AlwaysClamp);
            ImGui::PopStyleVar();
            ImGui::EndGroup();

            ImGui::TableNextColumn();
            ImGui::BeginGroup();
            ImGui::Text("Time: %.3fs", player_.time);
            ImGui::SameLine();
            if (ImGui::Button("Marker")) {
                PushUndo();
                Marker m; m.time = player_.time; m.label = "M" + std::to_string(clip_.markers.size() + 1);
                clip_.markers.push_back(m);
            }
            ImGui::SameLine();
            ImGui::SetNextItemWidth(120);
            ImGui::DragFloat("Length", &clip_.length, 0.01f, 0.1f, 600.f, "%.2fs");
            ImGui::SameLine();
            ImGui::SetNextItemWidth(80);
            ImGui::DragFloat("FPS", &clip_.fps, 0.1f, 1.f, 240.f);
            ImGui::EndGroup();

            ImGui::TableNextColumn();
            ImGui::BeginGroup();
            if (ImGui::Button("+ Track")) {
                PushUndo();
                TrackScalar t; t.name = std::string("Track ") + std::to_string(clip_.scalarTracks.size() + 1);
                clip_.scalarTracks.push_back(t);
            }
            ImGui::SameLine();
            if (ImGui::Button("Clear Sel")) { clip_.clear_selection(); }
            ImGui::SameLine();
            if (ImGui::SmallButton("Undo")) { Undo(); } ImGui::SameLine();
            if (ImGui::SmallButton("Redo")) { Redo(); }
            ImGui::SameLine();
            ImGui::TextDisabled("|");
            ImGui::SameLine();
            ImGui::Text("Zoom");
            ImGui::SameLine();
            ImGui::SetNextItemWidth(90);
            ImGui::SliderFloat("H##zoom", &timeScale_, 0.05f, 20.0f, "%.2fx");
            ImGui::SameLine();
            ImGui::SetNextItemWidth(90);
            ImGui::SliderFloat("V##zoom", &valueScale_, 0.05f, 20.0f, "%.2fx");

#ifdef QUASAR_USE_NLOHMANN_JSON
            ImGui::SameLine(); if (ImGui::SmallButton("Copy JSON")) { auto j = ToJson(); ImGui::SetClipboardText(j.dump(2).c_str()); }
            ImGui::SameLine(); if (ImGui::SmallButton("Paste JSON")) { if (const char* txt = ImGui::GetClipboardText()) { PushUndo(); json j = json::parse(txt, nullptr, false); if (!j.is_discarded()) FromJson(j); } }
#else
            ImGui::SameLine(); if (ImGui::SmallButton("Copy State")) { ImGui::SetClipboardText(ExportText().c_str()); }
#endif
            ImGui::EndGroup();

            ImGui::EndTable();
        }
        ImGui::Separator();
    }

    void AnimationEditor::DrawTimeline() {
        ImVec2 start = ImGui::GetCursorScreenPos();
        ImVec2 end = ImVec2(start.x + ImGui::GetContentRegionAvail().x, start.y + timelineHeight_);
        ImDrawList* dl = ImGui::GetWindowDrawList();

        dl->AddRectFilled(start, end, grid_.timeline);

        const float visible = (end.x - start.x) / (pixelsPerSecond_ * timeScale_);
        const float tStart = timeOffset_;
        const float tEnd = timeOffset_ + visible;

        float majorStep = ChooseNiceStep(visible);
        float minorStep = majorStep / 5.f;

        int imin = (int)std::floor(tStart / minorStep);
        int imax = (int)std::ceil(tEnd / minorStep);
        for (int i = imin; i <= imax; ++i) {
            float t = i * minorStep;
            float x = start.x + (t - timeOffset_) * pixelsPerSecond_ * timeScale_;
            dl->AddLine(ImVec2(x, start.y), ImVec2(x, end.y), grid_.gridMinor);
        }

        imin = (int)std::floor(tStart / majorStep);
        imax = (int)std::ceil(tEnd / majorStep);
        for (int i = imin; i <= imax; ++i) {
            float t = i * majorStep;
            float x = start.x + (t - timeOffset_) * pixelsPerSecond_ * timeScale_;
            dl->AddLine(ImVec2(x, start.y), ImVec2(x, end.y), grid_.gridMajor, 1.5f);
            char buf[32]; std::snprintf(buf, 32, "%.2f", t);
            dl->AddText(ImVec2(x + 4, start.y + 6), ImGui::GetColorU32(ImGuiCol_Text), buf);
        }

        float cx = start.x + (player_.time - timeOffset_) * pixelsPerSecond_ * timeScale_;
        dl->AddLine(ImVec2(cx, start.y), ImVec2(cx, end.y), grid_.playhead, 2.0f);

        ImGui::Dummy(ImVec2(end.x - start.x, timelineHeight_));

        if (ImGui::IsItemHovered()) {
            const float wheel = ImGui::GetIO().MouseWheel;
            if (wheel != 0.f) {
                float mouseX = ImGui::GetIO().MousePos.x;
                float mouseT = timeOffset_ + (mouseX - start.x) / (pixelsPerSecond_ * timeScale_);
                timeScale_ = std::clamp(timeScale_ + wheel * 0.15f, 0.05f, 20.f);
                timeOffset_ = mouseT - (mouseX - start.x) / (pixelsPerSecond_ * timeScale_);
            }
            if (ImGui::IsMouseDragging(ImGuiMouseButton_Middle)) {
                timeOffset_ -= ImGui::GetIO().MouseDelta.x / (pixelsPerSecond_ * timeScale_);
            }
            if (ImGui::IsMouseDown(ImGuiMouseButton_Left)) {
                float mx = ImGui::GetIO().MousePos.x;
                player_.time = TimeFromPixel(mx, start.x);
                player_.time = std::clamp(player_.time, 0.f, clip_.length);
            }
        }
    }

    void AnimationEditor::DrawTracks() {
        ImGui::BeginChild("Tracks", ImVec2(0, 0), false, ImGuiWindowFlags_NoScrollWithMouse);

        ImGuiListClipper clipper;
        const float itemHeight = trackHeight_ + graphHeight_ + trackSpacing_ * 2.f;
        clipper.Begin((int)clip_.scalarTracks.size(), itemHeight);
        while (clipper.Step()) {
            for (int i = clipper.DisplayStart; i < clipper.DisplayEnd; ++i) {
                DrawTrack(clip_.scalarTracks[(size_t)i], i);
            }
        }

        ImGui::Dummy(ImVec2(1, 6));
        ImGui::EndChild();
    }

    void AnimationEditor::DrawTrack(TrackScalar& track, int index) {
        ImDrawList* dl = ImGui::GetWindowDrawList();
        const ImVec2 rowStart = ImGui::GetCursorScreenPos();
        const ImVec2 rowEnd = ImVec2(rowStart.x + ImGui::GetContentRegionAvail().x, rowStart.y + trackHeight_);

        const ImU32 rowBg = (index % 2 == 0)
            ? ImGui::GetColorU32(ImGuiCol_TableRowBg)
            : ImGui::GetColorU32(ImGuiCol_TableRowBgAlt);
        dl->AddRectFilled(rowStart, rowEnd, rowBg == 0 ? ImGui::GetColorU32(ImGuiCol_FrameBg) : rowBg, 8.f);
        dl->AddRect(rowStart, rowEnd, grid_.gridMajor, 8.f, 0, 1.0f);

        ImVec2 labelStart = rowStart;
        ImVec2 labelEnd = ImVec2(rowStart.x + labelWidth_, rowEnd.y);
        dl->AddRectFilled(labelStart, labelEnd, ImGui::GetColorU32(ImGuiCol_FrameBg), 8.f, ImDrawFlags_RoundCornersLeft);
        dl->AddText(ImVec2(labelStart.x + 8, labelStart.y + 8), ImGui::GetColorU32(ImGuiCol_Text), track.name.c_str());

        ImGui::SetCursorScreenPos(ImVec2(labelStart.x + 8, labelStart.y + 22));
        ImGui::PushID(&track);
        ImGui::Checkbox("Mute", &track.mute); ImGui::SameLine();
        ImGui::Checkbox("Solo", &track.solo); ImGui::SameLine();
        ImGui::Checkbox("Lock", &track.lock);
        ImGui::SetCursorScreenPos(ImVec2(labelStart.x + 8, labelStart.y + trackHeight_ - 22));
        ImGui::ColorEdit4("##color", &track.color.x,
            ImGuiColorEditFlags_NoInputs | ImGuiColorEditFlags_NoLabel | ImGuiColorEditFlags_NoTooltip);
        ImGui::PopID();

        ImVec2 laneStart = ImVec2(rowStart.x + labelWidth_, rowStart.y);
        ImVec2 laneEnd = rowEnd;
        DrawKeyframeLane(track, laneStart, laneEnd);

        ImVec2 graphStart = ImVec2(rowStart.x + labelWidth_, rowEnd.y + 4);
        ImVec2 graphSize = ImVec2(ImGui::GetContentRegionAvail().x - labelWidth_, graphHeight_);
        DrawGraph(track, graphStart, graphSize);

        ImGui::Dummy(ImVec2(0, graphHeight_ + trackSpacing_ * 2 + trackHeight_));
        ImGui::SetCursorScreenPos(ImVec2(rowStart.x, rowEnd.y + graphHeight_ + trackSpacing_ * 2));
    }

    void AnimationEditor::DrawKeyframeLane(TrackScalar& track, ImVec2 start, ImVec2 end) {
        ImDrawList* dl = ImGui::GetWindowDrawList();
        dl->AddRectFilled(start, end, ImGui::GetColorU32(ImGuiCol_FrameBgHovered), 6.f);

        const float midY = 0.5f * (start.y + end.y);

        for (const auto& m : clip_.markers) {
            float x = PixelFromTime(m.time, start.x);
            dl->AddLine(ImVec2(x, start.y), ImVec2(x, end.y), m.color, 2.f);
            if (!m.label.empty()) dl->AddText(ImVec2(x + 3, start.y + 2), ImGui::GetColorU32(ImGuiCol_Text), m.label.c_str());
        }

        KeyframeScalar* hovered = nullptr;
        for (size_t i = 0; i < track.curve.keys.size(); ++i) {
            auto& k = track.curve.keys[i];
            float x = PixelFromTime(k.time, start.x);
            ImVec2 p(x, midY);
            ImU32 fill = k.selected ? grid_.keySel : ImGui::ColorConvertFloat4ToU32(track.color);
            dl->AddCircleFilled(p, 6.5f, fill);
            dl->AddCircle(p, 6.5f, grid_.keyEdge, 2.0f);

            ImGui::SetCursorScreenPos(ImVec2(p.x - 8, p.y - 8));
            ImGui::InvisibleButton(("kbtn" + std::to_string((size_t)&k)).c_str(), ImVec2(16, 16));

            if (ImGui::IsItemHovered()) hovered = &k;

            if (ImGui::IsItemActive() && ImGui::IsMouseDragging(ImGuiMouseButton_Left) && !track.lock) {
                PushUndo();
                float newT = TimeFromPixel(ImGui::GetIO().MousePos.x, start.x);
                if (ImGui::GetIO().KeyShift) newT = SnapTime(newT);
                k.time = std::clamp(newT, 0.f, clip_.length);
                std::sort(track.curve.keys.begin(), track.curve.keys.end(), [](auto& a, auto& b) {return a.time < b.time; });
            }
            if (ImGui::IsItemClicked(ImGuiMouseButton_Left)) {
                if (!ImGui::GetIO().KeyCtrl) clip_.clear_selection();
                k.selected = true;
            }
            if (ImGui::IsItemClicked(ImGuiMouseButton_Right)) {
                ImGui::OpenPopup(("kctx" + std::to_string((size_t)&k)).c_str());
            }

            if (ImGui::BeginPopup(("kctx" + std::to_string((size_t)&k)).c_str())) {
                PushUndo();
                ImGui::Text("Keyframe"); ImGui::Separator();
                ImGui::DragFloat("Time", &k.time, 0.01f, 0.f, clip_.length, "%.3f");
                ImGui::DragFloat("Value", &k.value, 0.01f);
                int interp = (int)k.interp; ImGui::Combo("Interp", &interp, "Step\0Linear\0Bezier\0Hermite\0Catmull\0"); k.interp = (Interpolation)interp;
                int inM = (int)k.inMode;   ImGui::Combo("In", &inM, "Auto\0AutoClamp\0Linear\0Flat\0Free\0"); k.inMode = (TangentMode)inM;
                int outM = (int)k.outMode;  ImGui::Combo("Out", &outM, "Auto\0AutoClamp\0Linear\0Flat\0Free\0"); k.outMode = (TangentMode)outM;
                if (k.inMode == TangentMode::Free)  ImGui::DragFloat("In tan", &k.inTangent, 0.01f);
                if (k.outMode == TangentMode::Free) ImGui::DragFloat("Out tan", &k.outTangent, 0.01f);
                if (ImGui::MenuItem("Delete")) { track.curve.keys.erase(track.curve.keys.begin() + i); ImGui::EndPopup(); goto popup_end; }
                ImGui::EndPopup();
            popup_end:;
            }
        }

        ImGui::SetCursorScreenPos(start);
        ImGui::InvisibleButton("lane", ImVec2(end.x - start.x, end.y - start.y));
        if (ImGui::IsItemHovered()) {
            if (ImGui::IsMouseClicked(ImGuiMouseButton_Left) && !hovered) {
                PushUndo();
                float t = PixelToTimeUnderMouse(start.x);
                t = std::clamp(ImGui::GetIO().KeyShift ? SnapTime(t) : t, 0.f, clip_.length);
                track.curve.add(t, 0.5f);
            }
            if (ImGui::IsMouseDragging(ImGuiMouseButton_Right)) {
                if (!selBox_.active) { selBox_.active = true; selBox_.start = ImGui::GetIO().MouseClickedPos[1]; }
                selBox_.end = ImGui::GetIO().MousePos;
                ImRect r(ImMin(selBox_.start, selBox_.end), ImMax(selBox_.start, selBox_.end));
                for (auto& k : track.curve.keys) {
                    float x = PixelFromTime(k.time, start.x);
                    ImVec2 p(x, midY);
                    if (r.Contains(p)) k.selected = true;
                }
            }
            else if (selBox_.active) {
                selBox_ = {};
            }
        }

        if (selBox_.active) {
            ImRect r(ImMin(selBox_.start, selBox_.end), ImMax(selBox_.start, selBox_.end));
            dl->AddRect(r.Min, r.Max, ImGui::GetColorU32(ImGuiCol_PlotLinesHovered), 2.f, 0, 1.5f);
        }
    }

    void AnimationEditor::DrawGraph(TrackScalar& track, ImVec2 start, ImVec2 size) {
        ImDrawList* dl = ImGui::GetWindowDrawList();
        ImVec2 end = start + size;

        dl->AddRectFilled(start, end, ImGui::GetColorU32(ImGuiCol_FrameBg), 7.f);
        DrawValueGrid(start, end);

        if (!track.curve.keys.empty()) {
            const int stepsPerSeg = 48;
            for (size_t i = 0; i + 1 < track.curve.keys.size(); ++i) {
                const auto& k0 = track.curve.keys[i];
                const auto& k1 = track.curve.keys[i + 1];
                float t0 = k0.time, t1 = k1.time; if (t1 <= t0) continue;

                ImU32 col = (k0.selected || k1.selected) ? grid_.curveSel : ImGui::ColorConvertFloat4ToU32(track.color);

                if (k0.interp == Interpolation::Linear || k0.interp == Interpolation::Step) {
                    ImVec2 p0(PixelFromTime(t0, start.x), PixelFromValue(k0.value, start.y, size.y));
                    ImVec2 p1(PixelFromTime(t1, start.x), PixelFromValue(k1.value, start.y, size.y));
                    if (k0.interp == Interpolation::Step) {
                        ImVec2 mid(PixelFromTime(t1, start.x), p0.y);
                        dl->AddLine(p0, mid, col, 2.0f);
                        dl->AddLine(mid, p1, col, 2.0f);
                    }
                    else {
                        dl->AddLine(p0, p1, col, 2.0f);
                    }
                }
                else {
                    ImVec2 prev;
                    for (int s = 0; s <= stepsPerSeg; ++s) {
                        float u = (float)s / stepsPerSeg;
                        float t = t0 + u * (t1 - t0);
                        float v = track.curve.evaluate(t);
                        ImVec2 p(PixelFromTime(t, start.x), PixelFromValue(v, start.y, size.y));
                        if (s > 0) dl->AddLine(prev, p, col, 2.0f);
                        prev = p;
                    }
                }
            }
        }

        for (size_t i = 0; i < track.curve.keys.size(); ++i) {
            auto& k = track.curve.keys[i];
            ImVec2 p(PixelFromTime(k.time, start.x), PixelFromValue(k.value, start.y, size.y));
            ImU32 fill = k.selected ? grid_.keySel : grid_.keyFill;
            dl->AddCircleFilled(p, 6.0f, fill);
            dl->AddCircle(p, 6.0f, grid_.keyEdge, 1.8f);

            const float dt = 0.3f * 60.f / clip_.fps;
            if (k.inMode == TangentMode::Free) {
                ImVec2 h = ImVec2(PixelFromTime(k.time - dt, start.x),
                    PixelFromValue(k.value - k.inTangent * dt, start.y, size.y));
                dl->AddLine(p, h, ImGui::GetColorU32(ImGuiCol_Separator), 1.0f);
                dl->AddCircleFilled(h, 4.f, ImGui::GetColorU32(ImGuiCol_SeparatorActive));
            }
            if (k.outMode == TangentMode::Free) {
                ImVec2 h = ImVec2(PixelFromTime(k.time + dt, start.x),
                    PixelFromValue(k.value + k.outTangent * dt, start.y, size.y));
                dl->AddLine(p, h, ImGui::GetColorU32(ImGuiCol_Separator), 1.0f);
                dl->AddCircleFilled(h, 4.f, ImGui::GetColorU32(ImGuiCol_SeparatorActive));
            }

            ImGui::SetCursorScreenPos(ImVec2(p.x - 7, p.y - 7));
            ImGui::InvisibleButton(("gkbtn" + std::to_string((size_t)&k)).c_str(), ImVec2(14, 14));
            if (ImGui::IsItemActive() && ImGui::IsMouseDragging(ImGuiMouseButton_Left)) {
                PushUndo();
                float newT = TimeFromPixel(ImGui::GetIO().MousePos.x, start.x);
                float newV = ValueFromPixel(ImGui::GetIO().MousePos.y, start.y, size.y);
                if (ImGui::GetIO().KeyShift) { newT = SnapTime(newT); newV = SnapValue(newV); }
                k.time = std::clamp(newT, 0.f, clip_.length);
                k.value = newV;
                std::sort(track.curve.keys.begin(), track.curve.keys.end(), [](auto& a, auto& b) {return a.time < b.time; });
            }
            if (ImGui::IsItemClicked(ImGuiMouseButton_Left)) {
                if (!ImGui::GetIO().KeyCtrl) clip_.clear_selection();
                k.selected = true;
            }
        }

        ImGui::SetCursorScreenPos(start);
        ImGui::InvisibleButton("graph", size);
        if (ImGui::IsItemHovered()) {
            float wheel = ImGui::GetIO().MouseWheel;
            if (wheel != 0.f && ImGui::GetIO().KeyCtrl) {
                float mouseV = ValueFromPixel(ImGui::GetIO().MousePos.y, start.y, size.y);
                valueScale_ = std::clamp(valueScale_ + wheel * 0.15f, 0.05f, 20.f);
                valueOffset_ = mouseV - ((ImGui::GetIO().MousePos.y - start.y) / (size.y)) * (1.f / valueScale_);
            }
            else if (wheel != 0.f) {
                float mouseT = PixelToTimeUnderMouse(start.x);
                timeScale_ = std::clamp(timeScale_ + wheel * 0.15f, 0.05f, 20.f);
                timeOffset_ = mouseT - (ImGui::GetIO().MousePos.x - start.x) / (pixelsPerSecond_ * timeScale_);
            }
            if (ImGui::IsMouseDragging(ImGuiMouseButton_Middle)) {
                timeOffset_ -= ImGui::GetIO().MouseDelta.x / (pixelsPerSecond_ * timeScale_);
                valueOffset_ += (ImGui::GetIO().MouseDelta.y / (size.y)) * (1.f / valueScale_) * 2.f;
            }
        }
    }

    void AnimationEditor::DrawValueGrid(ImVec2 start, ImVec2 end) {
        ImDrawList* dl = ImGui::GetWindowDrawList();
        const float width = end.x - start.x;
        const float height = end.y - start.y;
        (void)width;

        float vMin = valueOffset_ - 1.f / valueScale_;
        float vMax = valueOffset_ + 1.f / valueScale_;
        float step = 0.25f;
        for (float v = std::floor(vMin / step) * step; v <= vMax + 1e-4f; v += step) {
            float y = PixelFromValue(v, start.y, height);
            bool major = std::abs(std::fmod(v, 1.f)) < 1e-3f;
            dl->AddLine(ImVec2(start.x, y), ImVec2(end.x, y), major ? grid_.gridMajor : grid_.gridMinor);
        }

        float visible = (end.x - start.x) / (pixelsPerSecond_ * timeScale_);
        float majorStep = ChooseNiceStep(visible);
        int imin = (int)std::floor(timeOffset_ / majorStep);
        int imax = (int)std::ceil((timeOffset_ + visible) / majorStep);
        for (int i = imin; i <= imax; ++i) {
            float t = i * majorStep;
            float x = PixelFromTime(t, start.x);
            dl->AddLine(ImVec2(x, start.y), ImVec2(x, end.y), grid_.gridMinor);
        }
    }

    void AnimationEditor::HandleShortcuts() {
        ImGuiIO& io = ImGui::GetIO();
        if (!ImGui::IsWindowFocused(ImGuiFocusedFlags_ChildWindows)) return;

        if (ImGui::IsKeyPressed(ImGuiKey_Delete)) {
            PushUndo();
            for (auto& t : clip_.scalarTracks) t.curve.remove_if_selected();
        }

        if (io.KeyCtrl && ImGui::IsKeyPressed(ImGuiKey_D)) {
            PushUndo();
            for (auto& t : clip_.scalarTracks) {
                std::vector<KeyframeScalar> dupe;
                for (auto& k : t.curve.keys) if (k.selected) dupe.push_back(k);
                for (auto k : dupe) { k.time += 0.05f; k.selected = false; t.curve.keys.push_back(k); }
                std::sort(t.curve.keys.begin(), t.curve.keys.end(), [](auto& a, auto& b) {return a.time < b.time; });
            }
        }

        if (io.KeyCtrl && ImGui::IsKeyPressed(ImGuiKey_Z)) { Undo(); }
        if (io.KeyCtrl && ImGui::IsKeyPressed(ImGuiKey_Y)) { Redo(); }

        if (io.KeyCtrl && ImGui::IsKeyPressed(ImGuiKey_C)) {
            for (auto& t : clip_.scalarTracks) {
                bool any = false; for (auto& k : t.curve.keys) if (k.selected) { any = true; break; }
                if (!any) continue;
                std::string s;
                for (auto& k : t.curve.keys) if (k.selected) {
                    char buf[128]; std::snprintf(buf, 128, "%.6f,%.6f\n", k.time, k.value); s += buf;
                }
                ImGui::SetClipboardText(s.c_str());
                break;
            }
        }

        if (io.KeyCtrl && ImGui::IsKeyPressed(ImGuiKey_V)) {
            const char* txt = ImGui::GetClipboardText(); if (!txt) return;
            PushUndo();
            if (clip_.scalarTracks.empty()) {
                TrackScalar t; t.name = "Track 1"; clip_.scalarTracks.push_back(t);
            }
            auto& t = clip_.scalarTracks[0];
            float t0 = player_.time;
            float firstTime = std::numeric_limits<float>::max();
            std::vector<std::pair<float, float>> pairs;
            const char* p = txt;
            while (*p) {
                char* end = nullptr; float a = strtof(p, &end); if (end == p) break; p = end;
                if (*p == ',' || *p == ';' || *p == '\t') ++p; float b = strtof(p, &end); p = end;
                while (*p && *p != '\n' && *p != '\r') ++p; while (*p == '\n' || *p == '\r') ++p;
                pairs.emplace_back(a, b); if (a < firstTime) firstTime = a;
            }
            for (auto& pr : pairs) t.curve.add(t0 + (pr.first - firstTime), pr.second);
        }
    }

    void AnimationEditor::PushUndo() {
        redoStack_.clear();
        undoStack_.push_back({ clip_, timeOffset_, timeScale_, valueOffset_, valueScale_, player_.time });
        if (undoStack_.size() > 64) undoStack_.erase(undoStack_.begin());
    }

    void AnimationEditor::Undo() {
        if (undoStack_.empty()) return;
        redoStack_.push_back({ clip_, timeOffset_, timeScale_, valueOffset_, valueScale_, player_.time });
        auto s = undoStack_.back(); undoStack_.pop_back();
        clip_ = s.state; timeOffset_ = s.tOffset; timeScale_ = s.tScale; valueOffset_ = s.vOffset; valueScale_ = s.vScale; player_.time = s.playhead;
    }

    void AnimationEditor::Redo() {
        if (redoStack_.empty()) return;
        undoStack_.push_back({ clip_, timeOffset_, timeScale_, valueOffset_, valueScale_, player_.time });
        auto s = redoStack_.back(); redoStack_.pop_back();
        clip_ = s.state; timeOffset_ = s.tOffset; timeScale_ = s.tScale; valueOffset_ = s.vOffset; valueScale_ = s.vScale; player_.time = s.playhead;
    }

    float AnimationEditor::PixelFromTime(float t, float x0) const {
        return x0 + (t - timeOffset_) * pixelsPerSecond_ * timeScale_;
    }
    float AnimationEditor::TimeFromPixel(float x, float x0) const {
        return timeOffset_ + (x - x0) / (pixelsPerSecond_ * timeScale_);
    }

    float AnimationEditor::PixelFromValue(float v, float y0, float h) const {
        float norm = (v - valueOffset_) * valueScale_;
        return y0 + (1.f - norm) * (h - 12.f) + 6.f;
    }
    float AnimationEditor::ValueFromPixel(float y, float y0, float h) const {
        float norm = 1.f - ((y - y0 - 6.f) / (h - 12.f));
        return (norm / valueScale_) + valueOffset_;
    }

    float AnimationEditor::PixelToTimeUnderMouse(float x0) const {
        return TimeFromPixel(ImGui::GetIO().MousePos.x, x0);
    }

    float AnimationEditor::ChooseNiceStep(float visibleSeconds) {
        static const float steps[] = { 0.01f, 0.02f, 0.05f, 0.1f, 0.2f, 0.5f, 1.f, 2.f, 5.f, 10.f, 20.f, 30.f, 60.f, 120.f };
        float target = std::max(visibleSeconds / 8.f, 0.001f);
        for (float s : steps) if (s >= target) return s;
        return 120.f;
    }

    float AnimationEditor::SnapTime(float t) const {
        float visible = ImGui::GetContentRegionAvail().x / (pixelsPerSecond_ * timeScale_);
        float step = ChooseNiceStep(std::max(visible, 0.001f));
        return std::round(t / step) * step;
    }
    float AnimationEditor::SnapValue(float v) const {
        return std::round(v / 0.1f) * 0.1f;
    }

#ifdef QUASAR_USE_NLOHMANN_JSON
    nlohmann::json AnimationEditor::ToJson() const {
        json j;
        j["name"] = clip_.name;
        j["length"] = clip_.length;
        j["fps"] = clip_.fps;
        j["tracks"] = json::array();
        for (auto& t : clip_.scalarTracks) {
            json jt;
            jt["name"] = t.name;
            jt["color"] = { t.color.x, t.color.y, t.color.z, t.color.w };
            jt["mute"] = t.mute; jt["solo"] = t.solo; jt["lock"] = t.lock; jt["visible"] = t.visible;
            jt["pre"] = (int)t.curve.pre; jt["post"] = (int)t.curve.post;
            jt["keys"] = json::array();
            for (auto& k : t.curve.keys) {
                jt["keys"].push_back({
                    {"time", k.time}, {"value", k.value},
                    {"interp",(int)k.interp},
                    {"inMode",(int)k.inMode}, {"outMode",(int)k.outMode},
                    {"inTangent",k.inTangent}, {"outTangent",k.outTangent}
                    });
            }
            j["tracks"].push_back(jt);
        }
        j["markers"] = json::array();
        for (auto& m : clip_.markers) {
            j["markers"].push_back({ {"time", m.time}, {"color", m.color}, {"label", m.label} });
        }
        return j;
    }

    void AnimationEditor::FromJson(const nlohmann::json& j) {
        if (j.is_discarded()) return;
        clip_.name = j.value("name", "Clip");
        clip_.length = j.value("length", 5.f);
        clip_.fps = j.value("fps", 60.f);
        clip_.scalarTracks.clear();
        for (auto& jt : j.value("tracks", json::array())) {
            TrackScalar t;
            t.name = jt.value("name", "Track");
            if (jt.contains("color")) {
                auto c = jt["color"];
                if (c.is_array() && c.size() == 4) t.color = ImVec4(c[0], c[1], c[2], c[3]);
            }
            t.mute = jt.value("mute", false);
            t.solo = jt.value("solo", false);
            t.lock = jt.value("lock", false);
            t.visible = jt.value("visible", true);
            t.curve.pre = (Extrapolation)jt.value("pre", (int)Extrapolation::Hold);
            t.curve.post = (Extrapolation)jt.value("post", (int)Extrapolation::Hold);
            for (auto& jk : jt.value("keys", json::array())) {
                KeyframeScalar k;
                k.time = jk.value("time", 0.f);
                k.value = jk.value("value", 0.f);
                k.interp = (Interpolation)jk.value("interp", (int)Interpolation::Linear);
                k.inMode = (TangentMode)jk.value("inMode", (int)TangentMode::Auto);
                k.outMode = (TangentMode)jk.value("outMode", (int)TangentMode::Auto);
                k.inTangent = jk.value("inTangent", 0.f);
                k.outTangent = jk.value("outTangent", 0.f);
                t.curve.keys.push_back(k);
            }
            std::sort(t.curve.keys.begin(), t.curve.keys.end(), [](auto& a, auto& b) {return a.time < b.time; });
            clip_.scalarTracks.push_back(t);
        }
        clip_.markers.clear();
        for (auto& jm : j.value("markers", json::array())) {
            Marker m; m.time = jm.value("time", 0.f);
            m.color = jm.value("color", IM_COL32(255, 64, 64, 255));
            m.label = jm.value("label", "");
            clip_.markers.push_back(m);
        }
    }
#else
    std::string AnimationEditor::ExportText() const {
        std::string s; char buf[256];
        std::snprintf(buf, 256, "Clip %s length=%.3f fps=%.2f\n", clip_.name.c_str(), clip_.length, clip_.fps); s += buf;
        for (size_t i = 0; i < clip_.scalarTracks.size(); ++i) {
            const auto& t = clip_.scalarTracks[i];
            std::snprintf(buf, 256, "Track %zu %s keys=%zu\n", i, t.name.c_str(), t.curve.keys.size()); s += buf;
            for (auto& k : t.curve.keys) {
                std::snprintf(buf, 256, "  t=%.6f v=%.6f interp=%d in=%d out=%d inTan=%.4f outTan=%.4f\n",
                    k.time, k.value, (int)k.interp, (int)k.inMode, (int)k.outMode, k.inTangent, k.outTangent);
                s += buf;
            }
        }
        for (auto& m : clip_.markers) { std::snprintf(buf, 256, "Marker t=%.3f %s\n", m.time, m.label.c_str()); s += buf; }
        return s;
    }
#endif
}
