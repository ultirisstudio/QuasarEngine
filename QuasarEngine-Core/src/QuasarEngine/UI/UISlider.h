#pragma once

#include "UIElement.h"
#include "UIRenderer.h"
#include <algorithm>

namespace QuasarEngine
{
    class UISlider : public UIElement {
    public:
        using UIElement::UIElement;
        float min = 0.f, max = 1.f, value = 0.5f;
        bool dragging = false;

        explicit UISlider(std::string id) : UIElement(std::move(id)) { Init(); }
        UISlider() : UIElement("slider") { Init(); }

        void Measure(UILayoutContext&) override
        {
            if (Transform().size.x <= 0) Transform().size.x = 160.f;
            if (Transform().size.y <= 0) Transform().size.y = 24.f;
        }

        bool OnPointer(const UIPointerEvent& ev) override
        {
            if (!IsEnabled()) return false;
            if (ev.down && HitTest(ev.x, ev.y)) { dragging = true; setFromX(ev.x); return true; }
            if (ev.move && dragging) { setFromX(ev.x); return true; }
            if (ev.up) dragging = false;
            return false;
        }

        bool OnKey(const UIKeyEvent& ev) override
        {
            if (!IsEnabled() || !IsFocused() || !ev.down) return false;
            const int KEY_LEFT = 263, KEY_RIGHT = 262;
            float step = (max - min) * 0.02f;
            if (ev.key == KEY_LEFT) { value = std::max(min, value - step); return true; }
            if (ev.key == KEY_RIGHT) { value = std::min(max, value + step); return true; }
            return false;
        }

        void BuildDraw(UIRenderContext& ctx) override
        {
            Rect r = Transform().rect;
            Rect track{ r.x + m_Style.padding, r.y + r.h * 0.5f - 3, r.w - 2 * m_Style.padding, 6 };
            ctx.batcher->PushRect(track, ctx.whiteTex, PackRGBA8({ 0.25f,0.27f,0.30f,1 }), nullptr);
            float t = (value - min) / std::max(0.0001f, (max - min));
            Rect fill = track; fill.w = track.w * t;
            ctx.batcher->PushRect(fill, ctx.whiteTex, PackRGBA8({ 0.12f,0.55f,0.85f,1 }), nullptr);
            float tx = track.x + track.w * t;
            Rect thumb{ tx - 6, r.y + r.h * 0.5f - 8, 12, 16 };
            ctx.batcher->PushRect(thumb, ctx.whiteTex, PackRGBA8({ 0.9f,0.9f,0.9f,1 }), nullptr);
        }

    private:
        void setFromX(float px)
        {
            Rect r = Transform().rect;
            float trackL = r.x + m_Style.padding;
            float trackR = r.x + r.w - m_Style.padding;
            float t = 0.f;
            if (trackR > trackL) t = (px - trackL) / (trackR - trackL);
            t = std::clamp(t, 0.f, 1.f);
            value = min + t * (max - min);
        }

        void Init() { m_Style.padding = 8.f; SetFocusable(true); }
    };
}
