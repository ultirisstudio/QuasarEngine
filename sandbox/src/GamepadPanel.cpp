#include "GamepadPanel.h"

#include <quasar/core/Application.h>
#include <quasar/input/GamepadCodes.h>
#include <quasar/input/GamepadState.h>
#include <quasar/input/Gamepads.h>

#include <glm/vec2.hpp>
#include <imgui.h>
#include <spdlog/fmt/fmt.h>

#include <algorithm>
#include <array>
#include <chrono>
#include <cmath>
#include <cstddef>
#include <optional>
#include <span>
#include <string>
#include <tuple>
#include <utility>
#include <vector>

namespace Quasar::Sandbox
{
    namespace
    {
        constexpr float ViewWidth = 480.0f;
        constexpr float ViewHeight = 340.0f;
        constexpr float ViewSpacing = 40.0f;
        constexpr float TotalWidth = (ViewWidth * 2.0f) + ViewSpacing;
        constexpr float ViewTop = 2.0f;
        constexpr float LineThickness = 1.5f;
        constexpr int CurveSteps = 12;
        constexpr float StickTravel = 11.5f;
        constexpr float FingerRadius = 5.0f;
        constexpr float TouchSensorAlpha = 0.35f;
        constexpr float DetailsWidth = 290.0f;

        constexpr std::array<const char*, GamepadButtonCount> ButtonNames{"A",
                                                                          "B",
                                                                          "X",
                                                                          "Y",
                                                                          "Back",
                                                                          "Guide",
                                                                          "Start",
                                                                          "Left stick",
                                                                          "Right stick",
                                                                          "Left bumper",
                                                                          "Right bumper",
                                                                          "D-pad up",
                                                                          "D-pad down",
                                                                          "D-pad left",
                                                                          "D-pad right",
                                                                          "Misc 1",
                                                                          "Right paddle 1",
                                                                          "Left paddle 1",
                                                                          "Right paddle 2",
                                                                          "Left paddle 2",
                                                                          "Touchpad",
                                                                          "Misc 2",
                                                                          "Misc 3",
                                                                          "Misc 4",
                                                                          "Misc 5",
                                                                          "Misc 6"};

        constexpr std::array<const char*, GamepadTouchSensorCount> TouchSensorNames{"Left stick", "Right stick", "Left grip", "Right grip"};

        constexpr std::array<ImVec2, 30> FrontOutline{{{0.0f, 2.5f},      {-77.0f, 2.5f},    {-119.0f, 1.5f},   {-141.0f, 4.0f},   {-159.0f, 12.0f},
                                                       {-172.0f, 24.0f},  {-181.0f, 35.0f},  {-188.0f, 50.0f},  {-195.0f, 72.0f},  {-203.0f, 100.0f},
                                                       {-211.0f, 130.0f}, {-218.0f, 160.0f}, {-223.0f, 190.0f}, {-227.0f, 222.0f}, {-228.0f, 250.0f},
                                                       {-226.0f, 277.0f}, {-221.0f, 295.0f}, {-213.0f, 310.0f}, {-201.0f, 322.0f}, {-185.0f, 330.0f},
                                                       {-171.0f, 332.0f}, {-158.0f, 330.0f}, {-149.0f, 323.0f}, {-143.0f, 313.0f}, {-138.0f, 299.0f},
                                                       {-132.0f, 282.0f}, {-124.0f, 268.0f}, {-113.0f, 260.0f}, {-97.0f, 258.0f},  {0.0f, 258.0f}}};

        constexpr std::array<ImVec2, 15> FrontGripLine{{{-95.0f, 103.5f},
                                                        {-115.0f, 106.0f},
                                                        {-133.0f, 110.0f},
                                                        {-145.0f, 116.0f},
                                                        {-155.0f, 127.0f},
                                                        {-162.0f, 142.0f},
                                                        {-167.0f, 165.0f},
                                                        {-172.0f, 193.0f},
                                                        {-176.0f, 225.0f},
                                                        {-179.0f, 250.0f},
                                                        {-181.0f, 275.0f},
                                                        {-181.0f, 295.0f},
                                                        {-179.0f, 312.0f},
                                                        {-176.0f, 323.0f},
                                                        {-171.0f, 332.0f}}};

        constexpr std::array<ImVec2, 7> FrontBumperLine{
            {{-170.0f, 21.0f}, {-163.0f, 16.0f}, {-155.0f, 13.5f}, {-145.0f, 10.5f}, {-133.0f, 7.5f}, {-121.0f, 5.5f}, {-107.0f, 3.5f}}};

        constexpr std::array<ImVec2, 12> DpadOutline{{{-152.5f, 27.0f},
                                                      {-132.0f, 27.0f},
                                                      {-132.0f, 49.0f},
                                                      {-109.0f, 49.0f},
                                                      {-109.0f, 71.0f},
                                                      {-132.0f, 71.0f},
                                                      {-132.0f, 93.0f},
                                                      {-152.5f, 93.0f},
                                                      {-152.5f, 71.0f},
                                                      {-176.0f, 71.0f},
                                                      {-176.0f, 49.0f},
                                                      {-152.5f, 49.0f}}};

        constexpr std::array<ImVec2, 4> LeftTrackpad{{{-130.2f, 124.1f}, {-33.2f, 139.7f}, {-42.0f, 237.3f}, {-143.6f, 220.8f}}};

        constexpr std::array<ImVec2, 33> BackSide{
            {{-182.0f, 40.0f},  {-188.0f, 50.0f},  {-192.5f, 60.0f},  {-196.0f, 70.0f},  {-201.5f, 90.0f},  {-207.0f, 110.0f}, {-212.5f, 130.0f},
             {-217.5f, 150.0f}, {-223.0f, 175.0f}, {-228.0f, 200.0f}, {-231.0f, 220.0f}, {-233.0f, 240.0f}, {-233.5f, 255.0f}, {-232.5f, 270.0f},
             {-229.0f, 290.0f}, {-225.0f, 300.0f}, {-220.0f, 310.0f}, {-212.5f, 320.0f}, {-206.5f, 325.0f}, {-198.0f, 330.0f}, {-186.5f, 334.0f},
             {-173.5f, 335.2f}, {-159.5f, 331.5f}, {-147.0f, 320.0f}, {-142.0f, 310.0f}, {-138.0f, 300.0f}, {-133.0f, 290.0f}, {-127.0f, 280.0f},
             {-119.5f, 271.0f}, {-110.5f, 266.5f}, {-100.5f, 265.0f}, {-71.5f, 264.7f},  {0.0f, 264.7f}}};

        constexpr std::array<ImVec2, 5> BackTop{{{0.0f, 44.0f}, {-71.5f, 44.0f}, {-88.5f, 44.0f}, {-96.5f, 46.5f}, {-102.5f, 51.0f}}};

        constexpr std::array<ImVec2, 19> Bumper{{{-182.0f, 40.0f},
                                                 {-177.5f, 31.0f},
                                                 {-171.5f, 20.0f},
                                                 {-159.5f, 10.0f},
                                                 {-145.5f, 3.5f},
                                                 {-129.5f, 1.8f},
                                                 {-114.5f, 2.3f},
                                                 {-105.5f, 5.5f},
                                                 {-101.0f, 13.0f},
                                                 {-100.5f, 28.0f},
                                                 {-101.0f, 43.0f},
                                                 {-106.5f, 52.0f},
                                                 {-116.5f, 59.5f},
                                                 {-133.5f, 68.0f},
                                                 {-151.5f, 71.0f},
                                                 {-166.5f, 71.0f},
                                                 {-175.5f, 68.0f},
                                                 {-179.5f, 61.0f},
                                                 {-180.5f, 51.0f}}};

        constexpr std::array<ImVec2, 14> Trigger{{{-176.5f, 42.0f},
                                                  {-171.5f, 34.0f},
                                                  {-159.5f, 28.0f},
                                                  {-141.5f, 23.5f},
                                                  {-121.5f, 21.0f},
                                                  {-109.5f, 22.5f},
                                                  {-103.0f, 29.0f},
                                                  {-103.5f, 38.0f},
                                                  {-110.5f, 43.5f},
                                                  {-131.5f, 46.5f},
                                                  {-153.5f, 49.5f},
                                                  {-169.5f, 54.0f},
                                                  {-176.0f, 55.0f},
                                                  {-178.5f, 49.5f}}};

        constexpr std::array<ImVec2, 13> UpperPaddle{{{-121.5f, 160.5f},
                                                      {-111.5f, 165.0f},
                                                      {-105.5f, 175.0f},
                                                      {-104.5f, 187.0f},
                                                      {-107.5f, 197.0f},
                                                      {-114.5f, 204.0f},
                                                      {-123.5f, 209.0f},
                                                      {-130.5f, 210.0f},
                                                      {-136.5f, 205.0f},
                                                      {-139.5f, 195.0f},
                                                      {-139.5f, 182.0f},
                                                      {-136.5f, 170.0f},
                                                      {-130.5f, 163.0f}}};

        struct Colors
        {
            ImU32 Line = 0;
            ImU32 Active = 0;
            ImU32 Touched = 0;
            ImU32 Background = 0;
        };

        struct View
        {
            ImDrawList* DrawList = nullptr;
            ImVec2 Origin{0.0f, 0.0f};
            float Scale = 1.0f;
            float Side = 1.0f;
            Colors Palette;

            ImVec2 ToScreen(ImVec2 point) const { return {Origin.x + (point.x * Side * Scale), Origin.y + (point.y * Scale)}; }
            float Thickness() const { return std::max(1.0f, LineThickness * Scale); }
            View Mirrored() const
            {
                View mirrored = *this;
                mirrored.Side = -Side;
                return mirrored;
            }
        };

        struct StickInput
        {
            float X = 0.0f;
            float Y = 0.0f;
            bool Pressed = false;
            bool Touched = false;
        };

        ImVec2 ToImVec2(glm::vec2 vector)
        {
            return {vector.x, vector.y};
        }

        ImVec2 Lerp(ImVec2 from, ImVec2 to, float amount)
        {
            return {from.x + ((to.x - from.x) * amount), from.y + ((to.y - from.y) * amount)};
        }

        ImVec2 MoveToward(ImVec2 from, ImVec2 to, float distance)
        {
            const float length = std::hypot(to.x - from.x, to.y - from.y);
            return Lerp(from, to, length > 0.0f ? std::min(1.0f, distance / length) : 0.0f);
        }

        ImVec2 BezierPoint(ImVec2 start, ImVec2 startControl, ImVec2 endControl, ImVec2 end, float t)
        {
            const float u = 1.0f - t;
            const float a = u * u * u;
            const float b = 3.0f * u * u * t;
            const float c = 3.0f * u * t * t;
            const float d = t * t * t;
            return {(a * start.x) + (b * startControl.x) + (c * endControl.x) + (d * end.x),
                    (a * start.y) + (b * startControl.y) + (c * endControl.y) + (d * end.y)};
        }

        std::vector<ImVec2> BuildCurve(const View& view, std::span<const ImVec2> points, bool closed)
        {
            const auto count = static_cast<std::ptrdiff_t>(points.size());
            const auto at = [&](std::ptrdiff_t index)
            {
                if (closed)
                {
                    return points[static_cast<std::size_t>(((index % count) + count) % count)];
                }
                return points[static_cast<std::size_t>(std::clamp<std::ptrdiff_t>(index, 0, count - 1))];
            };

            std::vector<ImVec2> line;
            line.push_back(view.ToScreen(points.front()));
            const std::ptrdiff_t segments = closed ? count : count - 1;
            for (std::ptrdiff_t index = 0; index < segments; ++index)
            {
                const ImVec2 previous = at(index - 1);
                const ImVec2 start = at(index);
                const ImVec2 end = at(index + 1);
                const ImVec2 next = at(index + 2);
                const ImVec2 startControl{start.x + ((end.x - previous.x) / 6.0f), start.y + ((end.y - previous.y) / 6.0f)};
                const ImVec2 endControl{end.x - ((next.x - start.x) / 6.0f), end.y - ((next.y - start.y) / 6.0f)};
                for (int step = 1; step <= CurveSteps; ++step)
                {
                    const float t = static_cast<float>(step) / static_cast<float>(CurveSteps);
                    line.push_back(view.ToScreen(BezierPoint(start, startControl, endControl, end, t)));
                }
            }
            if (closed)
            {
                line.pop_back();
            }
            return line;
        }

        void StrokeCurve(const View& view, std::span<const ImVec2> points, bool closed)
        {
            const std::vector<ImVec2> line = BuildCurve(view, points, closed);
            view.DrawList->AddPolyline(line.data(), static_cast<int>(line.size()), view.Palette.Line, closed ? ImDrawFlags_Closed : ImDrawFlags_None,
                                       view.Thickness());
        }

        void FillCurve(const View& view, std::span<const ImVec2> points, ImU32 color)
        {
            const std::vector<ImVec2> line = BuildCurve(view, points, true);
            view.DrawList->AddConcavePolyFilled(line.data(), static_cast<int>(line.size()), color);
        }

        void FillCurveLevel(const View& view, std::span<const ImVec2> points, float level, ImU32 color)
        {
            if (level <= 0.0f)
            {
                return;
            }
            const std::vector<ImVec2> line = BuildCurve(view, points, true);
            const auto [lowest, highest] = std::ranges::minmax(line, {}, &ImVec2::y);
            const auto [leftmost, rightmost] = std::ranges::minmax(line, {}, &ImVec2::x);
            const float top = highest.y - ((highest.y - lowest.y) * std::min(level, 1.0f));
            view.DrawList->PushClipRect({leftmost.x, top}, {rightmost.x, highest.y}, true);
            view.DrawList->AddConcavePolyFilled(line.data(), static_cast<int>(line.size()), color);
            view.DrawList->PopClipRect();
        }

        void PathRoundedPolygon(const View& view, std::span<const ImVec2> corners, float radius)
        {
            const std::size_t count = corners.size();
            for (std::size_t index = 0; index < count; ++index)
            {
                const ImVec2 previous = corners[(index + count - 1) % count];
                const ImVec2 corner = corners[index];
                const ImVec2 next = corners[(index + 1) % count];
                view.DrawList->PathLineTo(view.ToScreen(MoveToward(corner, previous, radius)));
                view.DrawList->PathBezierQuadraticCurveTo(view.ToScreen(corner), view.ToScreen(MoveToward(corner, next, radius)));
            }
        }

        void StrokeRoundedPolygon(const View& view, std::span<const ImVec2> corners, float radius)
        {
            PathRoundedPolygon(view, corners, radius);
            view.DrawList->PathStroke(view.Palette.Line, ImDrawFlags_Closed, view.Thickness());
        }

        void FillRoundedPolygon(const View& view, std::span<const ImVec2> corners, float radius, ImU32 color)
        {
            PathRoundedPolygon(view, corners, radius);
            view.DrawList->PathFillConvex(color);
        }

        void StrokeLine(const View& view, ImVec2 from, ImVec2 to)
        {
            view.DrawList->AddLine(view.ToScreen(from), view.ToScreen(to), view.Palette.Line, view.Thickness());
        }

        void StrokeCircle(const View& view, ImVec2 center, float radius, ImU32 color)
        {
            view.DrawList->AddCircle(view.ToScreen(center), radius * view.Scale, color, 0, view.Thickness());
        }

        void StrokeCircle(const View& view, ImVec2 center, float radius)
        {
            StrokeCircle(view, center, radius, view.Palette.Line);
        }

        void FillCircle(const View& view, ImVec2 center, float radius, ImU32 color)
        {
            view.DrawList->AddCircleFilled(view.ToScreen(center), radius * view.Scale, color);
        }

        void Button(const View& view, ImVec2 center, float radius, bool pressed)
        {
            if (pressed)
            {
                FillCircle(view, center, radius, view.Palette.Active);
            }
            StrokeCircle(view, center, radius);
        }

        void StrokeEllipse(const View& view, ImVec2 center, ImVec2 radius)
        {
            view.DrawList->AddEllipse(view.ToScreen(center), {radius.x * view.Scale, radius.y * view.Scale}, view.Palette.Line, 0.0f, 0, view.Thickness());
        }

        void FillEllipse(const View& view, ImVec2 center, ImVec2 radius, ImU32 color)
        {
            view.DrawList->AddEllipseFilled(view.ToScreen(center), {radius.x * view.Scale, radius.y * view.Scale}, color);
        }

        std::pair<ImVec2, ImVec2> ScreenRect(const View& view, ImVec2 center, ImVec2 size)
        {
            const ImVec2 screenCenter = view.ToScreen(center);
            const ImVec2 halfSize{size.x * view.Scale / 2.0f, size.y * view.Scale / 2.0f};
            return {{screenCenter.x - halfSize.x, screenCenter.y - halfSize.y}, {screenCenter.x + halfSize.x, screenCenter.y + halfSize.y}};
        }

        void RoundedRectButton(const View& view, ImVec2 center, ImVec2 size, float rounding, bool pressed)
        {
            const auto [min, max] = ScreenRect(view, center, size);
            if (pressed)
            {
                view.DrawList->AddRectFilled(min, max, view.Palette.Active, rounding * view.Scale);
            }
            view.DrawList->AddRect(min, max, view.Palette.Line, rounding * view.Scale, view.Thickness());
        }

        void PillButton(const View& view, ImVec2 center, ImVec2 size, bool pressed)
        {
            RoundedRectButton(view, center, size, std::min(size.x, size.y) / 2.0f, pressed);
        }

        void Stick(const View& view, ImVec2 center, const StickInput& input)
        {
            StrokeCircle(view, center, 29.0f, input.Touched ? view.Palette.Active : view.Palette.Line);
            StrokeCircle(view, center, 24.5f);

            const ImVec2 cap{center.x + (input.X * StickTravel * view.Side), center.y + (input.Y * StickTravel)};
            FillCircle(view, cap, 17.5f, input.Pressed ? view.Palette.Active : view.Palette.Background);
            StrokeCircle(view, cap, 17.5f);
        }

        void Dpad(const View& view, const GamepadState& state)
        {
            const auto arm = [&](GamepadButton button, ImVec2 min, ImVec2 max, ImDrawFlags corners)
            {
                if (state.IsPressed(button))
                {
                    view.DrawList->AddRectFilled(view.ToScreen(min), view.ToScreen(max), view.Palette.Active, 4.0f * view.Scale, corners);
                }
            };
            arm(GamepadButton::DPadUp, {-152.5f, 27.0f}, {-132.0f, 49.0f}, ImDrawFlags_RoundCornersTop);
            arm(GamepadButton::DPadDown, {-152.5f, 71.0f}, {-132.0f, 93.0f}, ImDrawFlags_RoundCornersBottom);
            arm(GamepadButton::DPadLeft, {-176.0f, 49.0f}, {-152.5f, 71.0f}, ImDrawFlags_RoundCornersLeft);
            arm(GamepadButton::DPadRight, {-132.0f, 49.0f}, {-109.0f, 71.0f}, ImDrawFlags_RoundCornersRight);
            StrokeRoundedPolygon(view, DpadOutline, 4.0f);
        }

        ImVec2 TrackpadPoint(const View& view, ImVec2 position)
        {
            const float u = view.Side > 0.0f ? position.x : 1.0f - position.x;
            const ImVec2 top = Lerp(LeftTrackpad[0], LeftTrackpad[1], u);
            const ImVec2 bottom = Lerp(LeftTrackpad[3], LeftTrackpad[2], u);
            return Lerp(top, bottom, position.y);
        }

        void Trackpad(const View& view, const GamepadTouchpad& touchpad, bool clicked)
        {
            if (clicked)
            {
                FillRoundedPolygon(view, LeftTrackpad, 18.0f, view.Palette.Active);
            }
            StrokeRoundedPolygon(view, LeftTrackpad, 18.0f);

            const ImU32 fingerColor = clicked ? view.Palette.Line : view.Palette.Active;
            for (const GamepadFinger& finger : touchpad.Fingers)
            {
                if (finger.Down)
                {
                    FillCircle(view, TrackpadPoint(view, ToImVec2(finger.Position)), FingerRadius * (1.0f + finger.Pressure), fingerColor);
                }
            }
        }

        void DrawFront(const View& view, const GamepadState& state)
        {
            const View right = view.Mirrored();
            for (const View& half : {view, right})
            {
                StrokeCurve(half, FrontOutline, false);
                StrokeCurve(half, FrontGripLine, false);
                StrokeCurve(half, FrontBumperLine, false);
            }
            StrokeLine(view, {-37.5f, 103.0f}, {37.5f, 103.0f});

            PillButton(view, {-82.5f, 29.0f}, {29.0f, 12.0f}, state.IsPressed(GamepadButton::Back));
            PillButton(right, {-82.5f, 29.0f}, {29.0f, 12.0f}, state.IsPressed(GamepadButton::Start));
            PillButton(view, {0.0f, 180.0f}, {40.0f, 16.0f}, state.IsPressed(GamepadButton::Misc1));
            Button(view, {0.0f, 60.0f}, 15.0f, state.IsPressed(GamepadButton::Guide));
            StrokeCircle(view, {0.0f, 28.0f}, 3.0f);

            Dpad(view, state);
            Button(view, {141.75f, 36.5f}, 14.0f, state.IsPressed(GamepadButton::Y));
            Button(view, {115.25f, 60.0f}, 14.0f, state.IsPressed(GamepadButton::X));
            Button(view, {168.25f, 60.0f}, 14.0f, state.IsPressed(GamepadButton::B));
            Button(view, {141.75f, 83.5f}, 14.0f, state.IsPressed(GamepadButton::A));

            Stick(view, {-66.5f, 92.3f},
                  {.X = state.GetAxis(GamepadAxis::LeftX),
                   .Y = state.GetAxis(GamepadAxis::LeftY),
                   .Pressed = state.IsPressed(GamepadButton::LeftThumb),
                   .Touched = state.IsTouched(GamepadTouchSensor::LeftStick)});
            Stick(right, {-66.5f, 92.3f},
                  {.X = state.GetAxis(GamepadAxis::RightX),
                   .Y = state.GetAxis(GamepadAxis::RightY),
                   .Pressed = state.IsPressed(GamepadButton::RightThumb),
                   .Touched = state.IsTouched(GamepadTouchSensor::RightStick)});

            Trackpad(view, state.Touchpads[0], state.IsPressed(GamepadButton::Touchpad));
            Trackpad(right, state.Touchpads[1], state.IsPressed(GamepadButton::Misc2));
        }

        void DrawBackHalf(const View& view, const GamepadState& state, GamepadButton bumper, GamepadAxis trigger, GamepadButton upperPaddle,
                          GamepadButton lowerPaddle, GamepadTouchSensor grip)
        {
            if (state.IsTouched(grip))
            {
                FillEllipse(view, {-190.0f, 215.0f}, {22.0f, 60.0f}, view.Palette.Touched);
            }
            if (state.IsPressed(bumper))
            {
                FillCurve(view, Bumper, view.Palette.Active);
            }
            FillCurve(view, Trigger, view.Palette.Background);
            FillCurveLevel(view, Trigger, state.GetAxis(trigger), view.Palette.Active);
            if (state.IsPressed(upperPaddle))
            {
                FillCurve(view, UpperPaddle, view.Palette.Active);
            }
            if (state.IsPressed(lowerPaddle))
            {
                FillEllipse(view, {-142.75f, 250.5f}, {14.0f, 24.0f}, view.Palette.Active);
            }

            StrokeCurve(view, BackSide, false);
            StrokeCurve(view, BackTop, false);
            StrokeCurve(view, Bumper, true);
            StrokeCurve(view, Trigger, true);
            StrokeCurve(view, UpperPaddle, true);
            StrokeEllipse(view, {-142.75f, 250.5f}, {14.0f, 24.0f});
            StrokeLine(view, {-100.5f, 8.0f}, {0.0f, 8.0f});
            StrokeLine(view, {-100.5f, 15.5f}, {0.0f, 15.5f});
            StrokeCircle(view, {-61.5f, 57.5f}, 5.0f);
            StrokeCircle(view, {-178.2f, 118.0f}, 5.0f);
            StrokeCircle(view, {-198.7f, 300.0f}, 5.3f);
        }

        void DrawBack(const View& view, const GamepadState& state)
        {
            DrawBackHalf(view, state, GamepadButton::RightBumper, GamepadAxis::RightTrigger, GamepadButton::RightPaddle1, GamepadButton::RightPaddle2,
                         GamepadTouchSensor::RightGrip);
            DrawBackHalf(view.Mirrored(), state, GamepadButton::LeftBumper, GamepadAxis::LeftTrigger, GamepadButton::LeftPaddle1, GamepadButton::LeftPaddle2,
                         GamepadTouchSensor::LeftGrip);

            RoundedRectButton(view, {0.0f, 15.0f}, {22.0f, 5.0f}, 2.5f, false);
            RoundedRectButton(view, {0.0f, 57.75f}, {55.0f, 15.5f}, 7.75f, false);
            for (const float x : {-20.0f, 0.0f, 20.0f})
            {
                StrokeCircle(view, {x, 57.75f}, 4.5f);
            }
        }

        const char* GetTypeName(GamepadType type)
        {
            switch (type)
            {
                case GamepadType::Standard:
                    return "Standard";
                case GamepadType::Xbox360:
                    return "Xbox 360";
                case GamepadType::XboxOne:
                    return "Xbox One";
                case GamepadType::PlayStation3:
                    return "PlayStation 3";
                case GamepadType::PlayStation4:
                    return "PlayStation 4";
                case GamepadType::PlayStation5:
                    return "PlayStation 5";
                case GamepadType::SwitchPro:
                    return "Switch Pro";
                case GamepadType::JoyConLeft:
                    return "Left Joy-Con";
                case GamepadType::JoyConRight:
                    return "Right Joy-Con";
                case GamepadType::JoyConPair:
                    return "Pair of Joy-Cons";
                case GamepadType::GameCube:
                    return "GameCube";
                case GamepadType::SteamController:
                    return "Steam Controller";
                case GamepadType::Unknown:
                    break;
            }
            return "Unknown";
        }

        std::string GetLabel(const Gamepads& gamepads, GamepadId gamepad)
        {
            return fmt::format("Gamepad {}: {}", static_cast<unsigned>(gamepad), gamepads.GetName(gamepad));
        }

        void DisabledText(const char* text)
        {
            ImGui::PushStyleColor(ImGuiCol_Text, ImGui::GetStyleColorVec4(ImGuiCol_TextDisabled));
            ImGui::TextUnformatted(text);
            ImGui::PopStyleColor();
        }

        void WrappedList(const std::vector<const char*>& names)
        {
            if (names.empty())
            {
                DisabledText("None");
                return;
            }
            std::string text;
            for (const char* name : names)
            {
                text += text.empty() ? name : fmt::format(", {}", name);
            }
            ImGui::PushTextWrapPos(0.0f);
            ImGui::TextUnformatted(text.c_str());
            ImGui::PopTextWrapPos();
        }

        void DrawSticks(const GamepadState& state)
        {
            ImGui::SeparatorText("Sticks");
            if (!ImGui::BeginTable("Sticks", 3, ImGuiTableFlags_SizingStretchSame))
            {
                return;
            }
            ImGui::TableSetupColumn("");
            ImGui::TableSetupColumn("X");
            ImGui::TableSetupColumn("Y");
            ImGui::TableHeadersRow();
            for (const auto& [name, x, y] :
                 {std::tuple{"Left", GamepadAxis::LeftX, GamepadAxis::LeftY}, std::tuple{"Right", GamepadAxis::RightX, GamepadAxis::RightY}})
            {
                ImGui::TableNextRow();
                ImGui::TableNextColumn();
                ImGui::TextUnformatted(name);
                ImGui::TableNextColumn();
                ImGui::TextUnformatted(fmt::format("{:+.2f}", state.GetAxis(x)).c_str());
                ImGui::TableNextColumn();
                ImGui::TextUnformatted(fmt::format("{:+.2f}", state.GetAxis(y)).c_str());
            }
            ImGui::EndTable();
        }

        void DrawTriggers(const GamepadState& state)
        {
            ImGui::SeparatorText("Triggers");
            if (!ImGui::BeginTable("Triggers", 2, ImGuiTableFlags_SizingStretchSame))
            {
                return;
            }
            for (const auto& [name, axis] : {std::pair{"Left", GamepadAxis::LeftTrigger}, std::pair{"Right", GamepadAxis::RightTrigger}})
            {
                ImGui::TableNextRow();
                ImGui::TableNextColumn();
                ImGui::TextUnformatted(name);
                ImGui::TableNextColumn();
                ImGui::TextUnformatted(fmt::format("{:.2f}", state.GetAxis(axis)).c_str());
            }
            ImGui::EndTable();
        }

        void DrawButtons(const GamepadState& state)
        {
            ImGui::SeparatorText("Buttons");
            std::vector<const char*> pressed;
            for (std::size_t button = 0; button < GamepadButtonCount; ++button)
            {
                if (state.Buttons.at(button))
                {
                    pressed.push_back(ButtonNames.at(button));
                }
            }
            WrappedList(pressed);
        }

        void DrawTouchSensors(const GamepadState& state)
        {
            ImGui::SeparatorText("Touch sensors");
            std::vector<const char*> touched;
            for (std::size_t sensor = 0; sensor < GamepadTouchSensorCount; ++sensor)
            {
                if (state.TouchSensors.at(sensor))
                {
                    touched.push_back(TouchSensorNames.at(sensor));
                }
            }
            WrappedList(touched);
        }

        void DrawTouchpads(const GamepadState& state, std::size_t touchpadCount)
        {
            ImGui::SeparatorText("Touchpads");
            bool anyFinger = false;
            if (ImGui::BeginTable("Fingers", 5, ImGuiTableFlags_SizingStretchSame))
            {
                for (const char* header : {"Pad", "Finger", "X", "Y", "Pressure"})
                {
                    ImGui::TableSetupColumn(header);
                }
                ImGui::TableHeadersRow();
                for (std::size_t touchpad = 0; touchpad < touchpadCount; ++touchpad)
                {
                    for (std::size_t finger = 0; finger < MaxGamepadTouchpadFingers; ++finger)
                    {
                        const GamepadFinger& touch = state.Touchpads.at(touchpad).Fingers.at(finger);
                        if (!touch.Down)
                        {
                            continue;
                        }
                        anyFinger = true;
                        ImGui::TableNextRow();
                        for (const std::string& cell : {fmt::format("{}", touchpad), fmt::format("{}", finger), fmt::format("{:.2f}", touch.Position.x),
                                                        fmt::format("{:.2f}", touch.Position.y), fmt::format("{:.2f}", touch.Pressure)})
                        {
                            ImGui::TableNextColumn();
                            ImGui::TextUnformatted(cell.c_str());
                        }
                    }
                }
                ImGui::EndTable();
            }
            if (!anyFinger)
            {
                DisabledText("No finger on the touchpads");
            }
        }

        GamepadState ReadState(const Gamepads& gamepads, GamepadId gamepad)
        {
            GamepadState state;
            for (std::size_t button = 0; button < GamepadButtonCount; ++button)
            {
                state.Buttons.at(button) = gamepads.IsButtonPressed(gamepad, static_cast<GamepadButton>(button));
            }
            for (std::size_t axis = 0; axis < GamepadAxisCount; ++axis)
            {
                state.Axes.at(axis) = gamepads.GetAxis(gamepad, static_cast<GamepadAxis>(axis));
            }
            for (std::size_t touchpad = 0; touchpad < MaxGamepadTouchpads; ++touchpad)
            {
                for (std::size_t finger = 0; finger < MaxGamepadTouchpadFingers; ++finger)
                {
                    state.Touchpads.at(touchpad).Fingers.at(finger) = gamepads.GetTouchpadFinger(gamepad, touchpad, finger);
                }
            }
            for (std::size_t sensor = 0; sensor < GamepadTouchSensorCount; ++sensor)
            {
                state.TouchSensors.at(sensor) = gamepads.IsTouched(gamepad, static_cast<GamepadTouchSensor>(sensor));
            }
            return state;
        }

        void DrawGamepad(const GamepadState& state)
        {
            const ImVec2 available = ImGui::GetContentRegionAvail();
            const float scale = std::max(0.1f, std::min(available.x / TotalWidth, available.y / ViewHeight));
            const ImVec2 start = ImGui::GetCursorScreenPos();
            const float left = start.x + ((available.x - (TotalWidth * scale)) / 2.0f);
            const float top = start.y + ((available.y - (ViewHeight * scale)) / 2.0f) + (ViewTop * scale);

            View front{.DrawList = ImGui::GetWindowDrawList(),
                       .Scale = scale,
                       .Palette = {.Line = ImGui::GetColorU32(ImGuiCol_Text),
                                   .Active = ImGui::GetColorU32(ImGuiCol_CheckMark),
                                   .Touched = ImGui::GetColorU32(ImGuiCol_CheckMark, TouchSensorAlpha),
                                   .Background = ImGui::GetColorU32(ImGuiCol_WindowBg)}};
            front.Origin = {left + (ViewWidth / 2.0f * scale), top};
            View back = front;
            back.Origin = {left + ((ViewWidth * 1.5f + ViewSpacing) * scale), top};

            DrawFront(front, state);
            DrawBack(back, state);

            ImGui::Dummy(available);
        }
    }

    void GamepadPanel::Draw()
    {
        ImGui::SetNextWindowSize({DetailsWidth + (TotalWidth * 0.75f), ViewHeight * 0.95f}, ImGuiCond_FirstUseEver);
        if (!ImGui::Begin("Gamepad", nullptr, ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse))
        {
            ImGui::End();
            return;
        }

        Gamepads& gamepads = Application::Get().GetGamepads();
        const std::optional<GamepadId> gamepad = SelectGamepad(gamepads);
        const GamepadState state = gamepad ? ReadState(gamepads, *gamepad) : GamepadState{};

        constexpr ImGuiTableFlags layoutFlags = ImGuiTableFlags_Resizable | ImGuiTableFlags_Reorderable | ImGuiTableFlags_BordersInnerV;
        if (ImGui::BeginTable("Layout", 2, layoutFlags))
        {
            ImGui::TableSetupColumn("Details", ImGuiTableColumnFlags_WidthFixed, DetailsWidth);
            ImGui::TableSetupColumn("Drawing", ImGuiTableColumnFlags_WidthStretch);
            ImGui::TableHeadersRow();
            ImGui::TableNextRow();

            ImGui::TableSetColumnIndex(0);
            ImGui::BeginChild("Details");
            if (gamepad)
            {
                DrawDetails(gamepads, *gamepad, state);
            }
            else
            {
                DisabledText("No gamepad connected");
            }
            ImGui::EndChild();

            ImGui::TableSetColumnIndex(1);
            DrawGamepad(state);

            ImGui::EndTable();
        }

        ImGui::End();
    }

    std::optional<GamepadId> GamepadPanel::SelectGamepad(const Gamepads& gamepads)
    {
        if (!gamepads.IsConnected(m_Selected))
        {
            std::optional<GamepadId> first;
            for (std::size_t index = 0; index < MaxGamepads && !first; ++index)
            {
                if (gamepads.IsConnected(static_cast<GamepadId>(index)))
                {
                    first = static_cast<GamepadId>(index);
                }
            }
            if (!first)
            {
                return std::nullopt;
            }
            m_Selected = *first;
            m_RumbleFailed = false;
        }

        ImGui::SetNextItemWidth(-1.0f);
        if (ImGui::BeginCombo("##Gamepad", GetLabel(gamepads, m_Selected).c_str()))
        {
            for (std::size_t index = 0; index < MaxGamepads; ++index)
            {
                const auto gamepad = static_cast<GamepadId>(index);
                if (gamepads.IsConnected(gamepad) && ImGui::Selectable(GetLabel(gamepads, gamepad).c_str(), gamepad == m_Selected))
                {
                    m_Selected = gamepad;
                    m_RumbleFailed = false;
                }
            }
            ImGui::EndCombo();
        }
        return m_Selected;
    }

    void GamepadPanel::DrawDetails(Gamepads& gamepads, GamepadId gamepad, const GamepadState& state)
    {
        ImGui::TextUnformatted(fmt::format("Type: {}", GetTypeName(gamepads.GetType(gamepad))).c_str());

        DrawSticks(state);
        DrawTriggers(state);
        DrawButtons(state);
        if (gamepads.GetType(gamepad) == GamepadType::SteamController)
        {
            DrawTouchSensors(state);
        }
        if (gamepads.GetTouchpadCount(gamepad) > 0)
        {
            DrawTouchpads(state, gamepads.GetTouchpadCount(gamepad));
        }

        ImGui::SeparatorText("Rumble");
        if (ImGui::Button("Light"))
        {
            m_RumbleFailed = !gamepads.Rumble(gamepad, {.LowFrequency = 0.25f, .HighFrequency = 0.25f, .Duration = std::chrono::milliseconds(300)});
        }
        ImGui::SameLine();
        if (ImGui::Button("Strong"))
        {
            m_RumbleFailed = !gamepads.Rumble(gamepad, {.LowFrequency = 1.0f, .HighFrequency = 1.0f, .Duration = std::chrono::milliseconds(1000)});
        }
        ImGui::SameLine();
        if (ImGui::Button("Stop"))
        {
            m_RumbleFailed = !gamepads.Rumble(gamepad, {});
        }
        if (m_RumbleFailed)
        {
            DisabledText("This gamepad cannot rumble");
        }
    }
}
