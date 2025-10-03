#include "qepch.h"

#include "UIButton.h"
#include "UIRenderer.h"

namespace QuasarEngine {
    bool UIButton::OnPointer(const UIPointerEvent& ev) {
        if (!enabled) return false;
        bool inside = HitTest(ev.x, ev.y);
        if (ev.move) hovered = inside;
        if (ev.down && inside) { pressed = true; return true; }
        if (ev.up) {
            bool wasPressed = pressed;
            pressed = false;
            if (wasPressed && inside && onClick) { onClick(); return true; }
        }
        return inside;
    }

    void UIButton::Measure(UILayoutContext& ctx) {
        float textW = 8.f * (float)label.size();
        float textH = 18.f;
        if (Transform().size.x <= 0) Transform().size.x = textW + m_Style.padding * 2.f;
        if (Transform().size.y <= 0) Transform().size.y = textH + m_Style.padding * 2.f;
    }

    void UIButton::BuildDraw(UIRenderContext& ctx) {
        UIColor bg = m_Style.bg;
        if (pressed)       bg = { 0.25f,0.27f,0.30f,1.f };
        else if (hovered)  bg = { 0.18f,0.20f,0.23f,1.f };
        ctx.batcher->PushRect(Transform().rect, ctx.whiteTex, PackRGBA8(bg), nullptr);

        Rect r = Transform().rect;
        float tx = r.x + m_Style.padding;
        float ty = r.y + m_Style.padding;
        ctx.DrawText(label.c_str(), tx, ty, m_Style.fg);
    }
}