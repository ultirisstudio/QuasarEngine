#pragma once

#include <vector>
#include <string>
#include <imgui/imgui.h>
#include <algorithm>
#include <cmath>
#include <imgui/imgui_internal.h>

namespace QuasarEngine
{
    enum class InterpolationType {
        Linear,
        Bezier,
        Hermite
    };

    struct Keyframe {
        float time = 0, value = 0;
        InterpolationType interpolation = InterpolationType::Linear;
        float inTangent = 0, outTangent = 0;
        bool selected = false;
    };

    struct AnimationTrack {
        std::string name = "Track";
        std::vector<Keyframe> keyframes;
        ImVec4 color = ImVec4(0.31f, 0.51f, 1.0f, 1.0f);
    };

    class AnimationEditorPanel {
    public:
        AnimationEditorPanel();
        ~AnimationEditorPanel() = default;

        void Update(double dt);
        void OnImGuiRender();

    private:
        std::vector<AnimationTrack> tracks;
        float currentTime = 0.0f;
        float timelineLength = 10.0f;
        float pixelsPerSecond = 120.0f;
        float timeOffset = 0.0f;
        float timeScale = 1.0f;
        bool isPlaying = false;

        AnimationTrack* selectedTrack = nullptr;
        Keyframe* hoveredKeyframe = nullptr;
        Keyframe* draggedKeyframe = nullptr;
        ImVec2 dragOffset;

        void DrawTimeline();
        void DrawTracks();
        void DrawTrack(AnimationTrack& track, int index, float y);
        void DrawGraphEditor(AnimationTrack& track, float y);
        void HandleShortcuts();
        void AddKeyframe(AnimationTrack& track, float time, float value);
        void RemoveSelectedKeyframes();
        void DeselectAllKeyframes();
        void DrawMiniToolbar();

        float trackHeight = 42.0f;
        float trackSpacing = 8.0f;
        float labelWidth = 110.0f;
        float timelineHeight = 40.0f;
        float graphHeight = 90.0f;
        ImU32 backgroundColor = IM_COL32(245, 247, 250, 240);
        ImU32 timelineColor = IM_COL32(230, 232, 237, 255);
        ImU32 keyframeColor = IM_COL32(66, 133, 244, 240);
        ImU32 keyframeSelectedColor = IM_COL32(255, 193, 7, 255);
        ImU32 keyframeBorder = IM_COL32(33, 33, 33, 190);
    };
}
