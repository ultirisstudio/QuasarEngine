#pragma once

#include "UIElement.h"
#include "UIRenderer.h"

namespace QuasarEngine
{
    struct UIMenuItem
    {
        std::string label;
        bool disabled = false;
        std::function<void()> onClick;
    };

    class UIMenu : public UIElement
    {
    public:
        using UIElement::UIElement;
        std::vector<UIMenuItem> items;
        bool open = false;

        explicit UIMenu(std::string id) : UIElement(std::move(id)) {}
        UIMenu() : UIElement("menu") {}

        void OpenAt(float x, float y) { open = true; Transform().pos = { x,y }; }
        void Close() { open = false; }

        void Measure(UILayoutContext& ctx) override {
            float w = 0.f;
            float lineH = ctx.font ? (ctx.font->Ascent() - ctx.font->Descent()) : 18.f;
            for (auto& it : items) {
                float tw = ctx.font ? ctx.font->MeasureTextWidthUTF8(it.label) : 8.f * (float)it.label.size();
                w = std::max(w, tw);
            }
            if (Transform().size.x <= 0) Transform().size.x = w + 16.f;
            if (Transform().size.y <= 0) Transform().size.y = (lineH + 6.f) * items.size() + 4.f;
        }

        bool OnPointer(const UIPointerEvent& ev) override {
            if (!open) return false;
            if (ev.down) {
                int idx = itemAt(ev.x, ev.y);
                if (idx >= 0 && idx < (int)items.size()) {
                    if (!items[idx].disabled && items[idx].onClick) items[idx].onClick();
                    Close();
                    return true;
                }
                else {
                    Close();
                }
            }
            return false;
        }

        void BuildDraw(UIRenderContext& ctx) override {
            if (!open) return;
            Rect r = Transform().rect;
            ctx.batcher->PushRect(r, ctx.whiteTex, PackRGBA8({ 0.12f,0.12f,0.12f,1 }), nullptr);
            float lineH = ctx.defaultFont ? (ctx.defaultFont->Ascent() - ctx.defaultFont->Descent()) : 18.f;
            float y = r.y + 2.f;
            for (size_t i = 0; i < items.size(); ++i) {
                Rect ir{ r.x + 2, y, r.w - 4, lineH + 6.f };
                ctx.CtxDrawText(items[i].label.c_str(), ir.x + 6, ir.y + 3, items[i].disabled ? UIColor{ 0.6f,0.6f,0.6f,1 } : m_Style.fg);
                y += ir.h;
            }
        }

    private:
        int itemAt(float px, float py) const {
            Rect r = Transform().rect;
            float lineH = 18.f + 6.f;
            int idx = (int)((py - (r.y + 2.f)) / lineH);
            if (idx < 0 || idx >= (int)items.size()) return -1;
            float x0 = r.x + 2, x1 = r.x + r.w - 2;
            if (px < x0 || px > x1) return -1;
            return idx;
        }
    };
}
