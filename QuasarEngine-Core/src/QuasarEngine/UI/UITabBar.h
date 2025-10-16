#pragma once

#include "UIElement.h"
#include "UIRenderer.h"

namespace QuasarEngine
{
    class UITabBar : public UIElement
    {
    public:
        using UIElement::UIElement;
        std::vector<std::string> labels;
        int selected = 0;
        float tabHeight = 28.f;
        float tabGap = 2.f;

        explicit UITabBar(std::string id) : UIElement(std::move(id)) {}
        UITabBar() : UIElement("tabbar") {}

        std::function<void(int)> onChanged;

        void Measure(UILayoutContext& ctx) override {
            m_TabWidths.clear();
            m_TabWidths.reserve(labels.size());

            float w = 0.f;
            for (auto& s : labels) {
                const float tw = ctx.font ? ctx.font->MeasureTextWidthUTF8(s) : 8.f * (float)s.size();
                const float tabW = tw + 24.f;
                m_TabWidths.push_back(tabW);
                w += tabW;
            }
            w += tabGap * std::max(0, (int)labels.size() - 1);

            if (Transform().size.x <= 0) Transform().size.x = std::max(120.f, w);
            if (Transform().size.y <= 0) Transform().size.y = tabHeight;

            if (!labels.empty()) selected = std::clamp(selected, 0, (int)labels.size() - 1);
            else selected = 0;
        }

        bool OnPointer(const UIPointerEvent& ev) override {
            if (!IsEnabled()) return false;
            if (ev.down && HitTest(ev.x, ev.y)) {
                const int idx = hit(ev.x);
                if (idx >= 0 && idx < (int)labels.size()) {
                    if (selected != idx) { selected = idx; if (onChanged) onChanged(selected); }
                    return true;
                }
            }
            return false;
        }

        void BuildDraw(UIRenderContext& ctx) override {
            const Rect r = Transform().rect;
            ctx.batcher->PushRect(r, ctx.whiteTex, PackRGBA8({ 0.15f,0.16f,0.18f,1 }), nullptr);

            float x = r.x;
            const float y = r.y, h = r.h;

            for (int i = 0; i < (int)labels.size(); ++i) {
                const float w = (i < (int)m_TabWidths.size()) ? m_TabWidths[i] : 80.f;
                Rect t{ x, y, w, h };
                const UIColor bg = (i == selected) ? UIColor{ 0.22f,0.24f,0.28f,1 } : UIColor{ 0.18f,0.19f,0.21f,1 };
                ctx.batcher->PushRect(t, ctx.whiteTex, PackRGBA8(bg), nullptr);

                const float tw = ctx.defaultFont ? ctx.defaultFont->MeasureTextWidthUTF8(labels[i]) : 8.f * (float)labels[i].size();
                const float th = ctx.defaultFont ? (ctx.defaultFont->Ascent() - ctx.defaultFont->Descent()) : 18.f;
                const float tx = t.x + (t.w - tw) * 0.5f;
                const float ty = t.y + (t.h - th) * 0.5f;
                ctx.DrawText(labels[i].c_str(), tx, ty, m_Style.fg);

                x += w + tabGap;
            }
        }

    private:
        std::vector<float> m_TabWidths;

        int hit(float px) const {
            float x = Transform().rect.x;
            for (int i = 0; i < (int)labels.size(); ++i) {
                const float w = (i < (int)m_TabWidths.size()) ? m_TabWidths[i] : 80.f;
                if (px >= x && px < x + w) return i;
                x += w + tabGap;
            }
            return -1;
        }
    };

    class UITabs : public UIElement
    {
    public:
        using UIElement::UIElement;
        std::shared_ptr<UITabBar> tabbar;

        explicit UITabs(std::string id) : UIElement(std::move(id)) {
            tabbar = std::make_shared<UITabBar>("tabbar");
            AddChild(tabbar);
            tabbar->onChanged = [&](int i) { m_Selected = i; };
        }
        UITabs() : UITabs("tabs") {}

        int m_Selected = 0;

        void Measure(UILayoutContext& ctx) override {
            tabbar->Measure(ctx);
            const float barH = tabbar->Transform().size.y > 0 ? tabbar->Transform().size.y : tabbar->tabHeight;

            float contentWmax = 0.f, contentHmax = 0.f;
            int contentCount = 0;
            for (auto& c : Children()) {
                if (c.get() == tabbar.get()) continue;
                c->Measure(ctx);
                contentWmax = std::max(contentWmax, c->Transform().size.x);
                contentHmax = std::max(contentHmax, c->Transform().size.y);
                ++contentCount;
            }

            if (Transform().size.x <= 0) {
                float w = std::max(300.f, contentWmax);
                Transform().size.x = w;
            }
            if (Transform().size.y <= 0) {
                float h = std::max(200.f, contentHmax);
                Transform().size.y = barH + h;
            }

            if (contentCount > 0) m_Selected = std::clamp(m_Selected, 0, contentCount - 1);
            else m_Selected = 0;
        }

        void Arrange(const Rect& parentRect) override {
            m_Transform.rect.x = parentRect.x + m_Transform.pos.x;
            m_Transform.rect.y = parentRect.y + m_Transform.pos.y;
            m_Transform.rect.w = m_Transform.size.x;
            m_Transform.rect.h = m_Transform.size.y;

            Rect r = m_Transform.rect;
            const float barH = tabbar->Transform().size.y > 0 ? tabbar->Transform().size.y : tabbar->tabHeight;

            tabbar->Transform().pos = { 0,0 };
            tabbar->Transform().size = { r.w, barH };
            tabbar->Arrange({ r.x, r.y, r.w, barH });

            float cy = r.y + barH;
            float ch = r.h - barH;

            int visIdx = 0;
            for (auto& c : Children()) {
                if (c.get() == tabbar.get()) continue;
                if (visIdx == m_Selected) {
                    c->Transform().pos = { 0,0 };
                    c->Transform().size = { r.w, ch };
                    c->Arrange({ r.x, cy, r.w, ch });
                }
                else {
                    c->Transform().pos = { 0,0 };
                    c->Transform().size = { 0,0 };
                    c->Arrange({ 0,0,0,0 });
                }
                ++visIdx;
            }
        }

        void BuildDraw(UIRenderContext& ctx) override {
            tabbar->BuildDraw(ctx);
            int visIdx = 0;
            for (auto& c : Children()) {
                if (c.get() == tabbar.get()) continue;
                if (visIdx == m_Selected) c->BuildDraw(ctx);
                ++visIdx;
            }
        }
    };
}
