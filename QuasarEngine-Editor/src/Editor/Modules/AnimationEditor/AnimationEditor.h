#pragma once

#include <cstdint>
#include <string>
#include <vector>
#include <utility>

#include <imgui/imgui.h>
#include <imgui/imgui_internal.h>

#ifdef QUASAR_USE_NLOHMANN_JSON
#include <nlohmann/json.hpp>
using nlohmann::json;
#endif

#include <glm/glm.hpp>
#include <glm/gtc/quaternion.hpp>
#include <glm/gtx/quaternion.hpp>

#include <Editor/Modules/IEditorModule.h>
#include <Editor/UndoStack.h>

namespace QuasarEngine
{
    enum class Interpolation : uint8_t { Step, Linear, Bezier, Hermite, CatmullRom };
    enum class TangentMode : uint8_t { Auto, AutoClamped, Linear, Flat, Free };
    enum class Extrapolation : uint8_t { Hold, Linear, Loop, PingPong };
    enum class TrackType : uint8_t { Scalar, Vec2, Vec3, Quaternion };

    struct KeyframeScalar {
        float time = 0.f;
        float value = 0.f;

        Interpolation interp = Interpolation::Linear;
        TangentMode   inMode = TangentMode::Auto;
        TangentMode   outMode = TangentMode::Auto;

        float inTangent = 0.f;
        float outTangent = 0.f;

        float tension = 0.f;
        float continuity = 0.f;
        float bias = 0.f;

        bool selected = false;
    };

    struct SegmentCache {
        int   index = -1;
        float t0 = 0.f;
        float t1 = 0.f;
    };

    class CurveScalar {
    public:
        std::vector<KeyframeScalar> keys;
        Extrapolation pre = Extrapolation::Hold;
        Extrapolation post = Extrapolation::Hold;

        CurveScalar() = default;

        bool  empty() const;
        void  clear();
        int   add(float time, float value);
        void  remove_if_selected();

        float evaluate(float t) const;

        static float wrap_repeat(float t, float start, float end);
        static float wrap_pingpong(float t, float start, float end);

    private:
        mutable SegmentCache hot_{};

        static float cubic_hermite(float u, float p0, float p1, float m0, float m1);
        static float catmull_rom(float p0, float p1, float p2, float p3, float u);

        float value_at_index_clamped(int idx) const;
        int   find_segment(float t) const;
        float resolve_tangent(const KeyframeScalar& k, int index, int direction) const;

        float linear_extrap_left(float t) const;
        float linear_extrap_right(float t) const;
    };

    struct TrackScalar {
        std::string name = "Track";
        ImVec4 color = ImVec4(0.31f, 0.51f, 1.0f, 1.0f);
        bool  mute = false;
        bool  solo = false;
        bool  lock = false;
        bool  visible = true;

        CurveScalar curve;
    };

    struct TrackVec3 {
        std::string name = "Vec3";
        ImVec4 color = ImVec4(0.25f, 0.80f, 0.60f, 1.0f);
        bool  mute = false, solo = false, lock = false, visible = true;

        CurveScalar x, y, z;

        glm::vec3 evaluate(float t) const;
    };

    struct TrackQuat {
        std::string name = "Rotation";
        ImVec4 color = ImVec4(0.94f, 0.52f, 0.36f, 1.0f);
        bool  mute = false, solo = false, lock = false, visible = true;

        CurveScalar x, y, z, w;

        glm::quat evaluate(float t) const;
    };

    struct Marker {
        float time = 0.f;
        ImU32 color = IM_COL32(255, 64, 64, 255);
        std::string label;
    };

    struct AnimationClip {
        std::string name = "Clip";
        float length = 5.f;
        float fps = 60.f;

        std::vector<TrackScalar> scalarTracks;

        std::vector<TrackVec3>   vec3Tracks;
        std::vector<TrackQuat>   quatTracks;

        std::vector<Marker> markers;

        void clear_selection();
        bool any_solo() const;
    };

    struct Player {
        float time = 0.f;
        float speed = 1.f;
        bool  playing = false;
        Extrapolation mode = Extrapolation::Loop;

        void update(double dt, const AnimationClip& clip);
        void play() { playing = true; }
        void pause() { playing = false; }
        void stop() { playing = false; time = 0.f; }
    };

    struct GridStyle {
        ImU32 bg = IM_COL32(247, 248, 251, 255);
        ImU32 timeline = IM_COL32(230, 232, 237, 255);
        ImU32 gridMajor = IM_COL32(190, 196, 205, 255);
        ImU32 gridMinor = IM_COL32(220, 224, 231, 255);
        ImU32 playhead = IM_COL32(220, 60, 60, 255);
        ImU32 curve = IM_COL32(60, 130, 240, 255);
        ImU32 curveSel = IM_COL32(255, 193, 7, 255);
        ImU32 keyFill = IM_COL32(66, 133, 244, 240);
        ImU32 keySel = IM_COL32(255, 193, 7, 255);
        ImU32 keyEdge = IM_COL32(33, 33, 33, 200);
        ImU32 marker = IM_COL32(255, 64, 64, 255);
    };

    class AnimationEditor : public IEditorModule {
    public:
        AnimationEditor(EditorContext& context);
        ~AnimationEditor() override;

        void Update(double dt) override;

        void RenderUI() override;

        AnimationClip& GetClip();
        const AnimationClip& GetClip() const;
        Player& GetPlayer();

#ifdef QUASAR_USE_NLOHMANN_JSON
        nlohmann::json ToJson() const;
        void           FromJson(const nlohmann::json& j);
#else
        std::string    ExportText() const;
#endif

    private:
        AnimationClip clip_;
        Player        player_;
        GridStyle     grid_;

        float pixelsPerSecond_ = 120.f;
        float timeOffset_ = 0.f;
        float timeScale_ = 1.f;

        float valueOffset_ = 0.f;
        float valueScale_ = 1.f;

        float trackHeight_ = 42.f;
        float graphHeight_ = 120.f;
        float trackSpacing_ = 8.f;
        float labelWidth_ = 140.f;
        float timelineHeight_ = 38.f;

        struct SelectionBox { bool active = false; ImVec2 start{}, end{}; } selBox_;
        bool draggingTimeline_ = false;

        struct Snapshot {
            AnimationClip state;
            float tOffset, tScale, vOffset, vScale;
            float playhead;
        };
        UndoStack<Snapshot> m_History{ 64 };

        void DrawToolbar();
        void DrawTimeline();
        void DrawTracks();
        void DrawTrack(TrackScalar& track, int index);
        void DrawKeyframeLane(TrackScalar& track, ImVec2 start, ImVec2 end);
        void DrawGraph(TrackScalar& track, ImVec2 start, ImVec2 size);
        void DrawValueGrid(ImVec2 start, ImVec2 end);

        void HandleShortcuts();

        void PushUndo();
        void Undo();
        void Redo();

        float PixelFromTime(float t, float x0) const;
        float TimeFromPixel(float x, float x0) const;
        float PixelFromValue(float v, float y0, float h) const;
        float ValueFromPixel(float y, float y0, float h) const;
        float PixelToTimeUnderMouse(float x0) const;

        static float ChooseNiceStep(float visibleSeconds);
        float SnapTime(float t) const;
        float SnapValue(float v) const;
    };

    inline bool CurveScalar::empty() const { return keys.empty(); }
    inline void CurveScalar::clear() { keys.clear(); hot_ = {}; }
    inline float CurveScalar::wrap_repeat(float t, float start, float end) {
        const float len = end - start; if (len <= 0.f) return start;
        float x = std::fmod(t - start, len); if (x < 0) x += len; return start + x;
    }
    inline float CurveScalar::wrap_pingpong(float t, float start, float end) {
        const float len = end - start; if (len <= 0.f) return start;
        float x = std::fmod(t - start, 2.f * len); if (x < 0) x += 2.f * len;
        if (x > len) x = 2.f * len - x; return start + x;
    }

    inline bool AnimationClip::any_solo() const {
        for (auto& t : scalarTracks) if (t.solo) return true;

        for (auto& t : vec3Tracks) if (t.solo) return true;
        for (auto& t : quatTracks) if (t.solo) return true;

        return false;
    }

    inline void Player::update(double dt, const AnimationClip& clip) {
        if (!playing) return;
        time += float(dt) * speed;
        if (time < 0.f || time > clip.length) {
            switch (mode) {
            case Extrapolation::Hold:    time = std::clamp(time, 0.f, clip.length); break;
            case Extrapolation::Loop:    time = CurveScalar::wrap_repeat(time, 0.f, clip.length); break;
            case Extrapolation::PingPong:time = CurveScalar::wrap_pingpong(time, 0.f, clip.length); break;
            case Extrapolation::Linear:  break;
            }
        }
    }

    inline AnimationClip& AnimationEditor::GetClip() { return clip_; }
    inline const AnimationClip& AnimationEditor::GetClip() const { return clip_; }
    inline Player& AnimationEditor::GetPlayer() { return player_; }

    inline glm::vec3 TrackVec3::evaluate(float t) const { return glm::vec3(x.evaluate(t), y.evaluate(t), z.evaluate(t)); }
    inline glm::quat TrackQuat::evaluate(float t) const {
        glm::quat q = glm::normalize(glm::quat(w.evaluate(t), x.evaluate(t), y.evaluate(t), z.evaluate(t)));
        return q;
    }

    inline void AnimationClip::clear_selection() {
        for (auto& ts : scalarTracks) for (auto& k : ts.curve.keys) k.selected = false;

        for (auto& tv : vec3Tracks) { for (auto& k : tv.x.keys) k.selected = false; for (auto& k : tv.y.keys) k.selected = false; for (auto& k : tv.z.keys) k.selected = false; }
        for (auto& tq : quatTracks) { for (auto& k : tq.x.keys) k.selected = false; for (auto& k : tq.y.keys) k.selected = false; for (auto& k : tq.z.keys) k.selected = false; for (auto& k : tq.w.keys) k.selected = false; }

    }
}
