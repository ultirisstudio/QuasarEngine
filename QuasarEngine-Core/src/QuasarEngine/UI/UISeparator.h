#pragma once

#include "UIElement.h"
#include "UIRenderer.h"

namespace QuasarEngine {
    class UISeparator : public UIElement {
    public:
        using UIElement::UIElement;
        float thickness = 1.f;
        UIColor color{ 0.63f,0.63f,0.67f,1.f };

        void Measure(UILayoutContext&) override {
            if (Transform().size.y <= 0) Transform().size.y = thickness;
            if (Transform().size.x <= 0) Transform().size.x = 120.f;
        }
        void BuildDraw(UIRenderContext& ctx) override {
            Rect r = Transform().rect;
            Rect line{ r.x, r.y + r.h * 0.5f - thickness * 0.5f, r.w, thickness };
            ctx.batcher->PushRect(line, ctx.whiteTex, PackRGBA8(color), nullptr);
        }
    };
}