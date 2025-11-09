#pragma once

#include "UIElement.h"
#include "UIRenderer.h"
#include <algorithm>

namespace QuasarEngine
{
    class UIProgressBar : public UIElement {
    public:
        using UIElement::UIElement;
        float min = 0.f, max = 1.f, value = 0.f;
        bool showText = true;

        explicit UIProgressBar(std::string id) : UIElement(std::move(id)) {}
        UIProgressBar() : UIElement("progress") {}

        void Measure(UILayoutContext&) override {
            if (Transform().size.x <= 0) Transform().size.x = 200.f;
            if (Transform().size.y <= 0) Transform().size.y = 20.f;
        }

        void BuildDraw(UIRenderContext& ctx) override {
            Rect r = Transform().rect;
            ctx.batcher->PushRect(r, ctx.whiteTex, PackRGBA8({ 0.2f,0.22f,0.25f,1 }), nullptr);
            float t = (value - min) / std::max(0.0001f, (max - min));
            Rect fill{ r.x + 1, r.y + 1, (r.w - 2) * std::clamp(t,0.f,1.f), r.h - 2 };
            ctx.batcher->PushRect(fill, ctx.whiteTex, PackRGBA8({ 0.12f,0.55f,0.85f,1 }), nullptr);

            if (showText && ctx.defaultFont) {
                char buf[64];
                std::snprintf(buf, sizeof(buf), "%.0f%%", t * 100.0f);
                float tw = ctx.defaultFont->MeasureTextWidthUTF8(buf);
                float th = ctx.defaultFont->Ascent() - ctx.defaultFont->Descent();
                float x = r.x + (r.w - tw) * 0.5f;
                float y = r.y + (r.h - th) * 0.5f;
                ctx.CtxDrawText(buf, x, y, m_Style.fg);
            }
        }
    };
}
