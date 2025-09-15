#include "qepch.h"

#include "UIElement.h"
#include "UIRenderer.h"

namespace QuasarEngine {
    void UIElement::Measure(UILayoutContext&) {
        if (m_Transform.size.x <= 0) m_Transform.size.x = 100.f;
        if (m_Transform.size.y <= 0) m_Transform.size.y = 24.f;
    }

    void UIElement::Arrange(const Rect& parentRect) {
        m_Transform.rect.x = parentRect.x + m_Transform.pos.x;
        m_Transform.rect.y = parentRect.y + m_Transform.pos.y;
        m_Transform.rect.w = m_Transform.size.x;
        m_Transform.rect.h = m_Transform.size.y;

        for (auto& c : m_Children) {
            c->Arrange(m_Transform.rect);
        }
    }

    void UIElement::BuildDraw(UIRenderContext& ctx) {
        if (m_Style.bg.a > 0.f && m_Transform.rect.w > 0 && m_Transform.rect.h > 0) {
            auto color = PackRGBA8(m_Style.bg);
            ctx.batcher->PushRect(m_Transform.rect, color, nullptr);
        }
        for (auto& c : m_Children) c->BuildDraw(ctx);
    }
}