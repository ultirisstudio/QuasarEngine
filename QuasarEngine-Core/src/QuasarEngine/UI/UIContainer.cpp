#include "qepch.h"

#include "UIContainer.h"
#include "UIRenderer.h"

namespace QuasarEngine {
    void UIContainer::Measure(UILayoutContext& ctx) {
        float w = 0.f, h = 0.f;
        for (auto& c : Children()) {
            c->Measure(ctx);
            if (layout == UILayoutType::Vertical) {
                w = std::max(w, c->Transform().size.x);
                h += c->Transform().size.y;
            }
            else {
                h = std::max(h, c->Transform().size.y);
                w += c->Transform().size.x;
            }
        }
        
        int count = (int)Children().size();
        if (count > 1) {
            if (layout == UILayoutType::Vertical) h += gap * (count - 1);
            else w += gap * (count - 1);
        }
        w += m_Style.padding * 2.f;
        h += m_Style.padding * 2.f;

        if (Transform().size.x <= 0) Transform().size.x = w;
        if (Transform().size.y <= 0) Transform().size.y = h;
    }

    void UIContainer::Arrange(const Rect& parentRect) {
        UIElement::Arrange(parentRect);

        float cursorX = m_Transform.rect.x + m_Style.padding;
        float cursorY = m_Transform.rect.y + m_Style.padding;

        for (size_t i = 0; i < Children().size(); ++i) {
            auto& c = Children()[i];
            auto cs = c->Transform().size;

            Rect childParent = m_Transform.rect;
            
            if (layout == UILayoutType::Vertical) {
                childParent.x = cursorX;
                childParent.y = cursorY;
                c->Transform().pos = { 0,0 };
                c->Transform().size = cs;
                c->Arrange({ childParent.x, childParent.y, cs.x, cs.y });
                cursorY += cs.y + ((i + 1 < Children().size()) ? gap : 0.f);
            }
            else {
                childParent.x = cursorX;
                childParent.y = cursorY;
                c->Transform().pos = { 0,0 };
                c->Transform().size = cs;
                c->Arrange({ childParent.x, childParent.y, cs.x, cs.y });
                cursorX += cs.x + ((i + 1 < Children().size()) ? gap : 0.f);
            }
        }
    }

    void UIContainer::BuildDraw(UIRenderContext& ctx) {
        if (m_Style.bg.a > 0.f) {
            ctx.batcher->PushRect(m_Transform.rect, ctx.whiteTex, PackRGBA8(m_Style.bg), nullptr);
        }
        for (auto& c : Children()) c->BuildDraw(ctx);
    }
}