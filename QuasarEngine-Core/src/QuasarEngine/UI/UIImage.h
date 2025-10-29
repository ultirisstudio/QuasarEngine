#pragma once

#include "UIElement.h"
#include "UIRenderer.h"

namespace QuasarEngine {
    class UIImage : public UIElement {
    public:
        using UIElement::UIElement;

        std::string textureId;
        bool keepAspect = true;
        UIColor tint{ 1,1,1,1 };

        void Measure(UILayoutContext& ctx) override {
            if (Transform().size.x <= 0) Transform().size.x = 128.f;
            if (Transform().size.y <= 0) Transform().size.y = 128.f;
        }

        void BuildDraw(UIRenderContext& ctx) override {
            ctx.batcher->PushRect(Transform().rect,
                UITexture{ textureId.empty() ? "ui:white" : textureId },
                PackRGBA8(tint),
                nullptr);
        }
    };
}
