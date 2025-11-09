#pragma once
#include "UIElement.h"
#include "UIRenderer.h"

namespace QuasarEngine
{
    struct Tooltip {
        std::string text;
        float x = 0, y = 0;
        float timeout = 1.0f;
        bool visible = false;
    };

    class UITooltipLayer : public UIElement {
    public:
        using UIElement::UIElement;
        Tooltip tip;

        explicit UITooltipLayer(std::string id) : UIElement(std::move(id)) {}
        UITooltipLayer() : UIElement("tooltip") {}

        void Show(const std::string& t, float x, float y) { tip.text = t; tip.x = x; tip.y = y; tip.visible = true; }
        void Hide() { tip.visible = false; }

        void Measure(UILayoutContext&) override {}
        void Arrange(const Rect& parentRect) override { m_Transform.rect = parentRect; }

        void BuildDraw(UIRenderContext& ctx) {
            if (!tip.visible || tip.text.empty()) return;

            const float tw = ctx.defaultFont ? ctx.defaultFont->MeasureTextWidthUTF8(tip.text) : 8.f * (float)tip.text.size();
            const float th = ctx.defaultFont ? (ctx.defaultFont->Ascent() - ctx.defaultFont->Descent()) : 18.f;

            const float padX = 6.f, padY = 4.f;
            float rx = tip.x + 12.f;
            float ry = tip.y - (th + padY * 2.f) - 12.f;

            if (ry < 0.f) ry = tip.y + 12.f;

            Rect r{ rx, ry, tw + padX * 2.f, th + padY * 2.f };
            ctx.batcher->PushRect(r, ctx.whiteTex, PackRGBA8({ 0,0,0,0.85f }), nullptr);
            ctx.CtxDrawText(tip.text.c_str(), r.x + padX, r.y + padY, { 1,1,1,1 });
        }

        bool HitTest(float /*x*/, float /*y*/) const override {
            return false;
        }
    };
}
