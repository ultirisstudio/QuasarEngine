#pragma once

#include "UIElement.h"
#include "UIRenderer.h"

namespace QuasarEngine
{
    class UICheckbox : public UIElement
    {
    public:
        using UIElement::UIElement;
        std::string label = "Checkbox";
        bool checked = false;

        explicit UICheckbox(std::string id) : UIElement(std::move(id)) { Init(); }
        UICheckbox() : UIElement("checkbox") { Init(); }

        void Measure(UILayoutContext& ctx) override
        {
            float box = 16.f;
            float tw = ctx.font ? ctx.font->MeasureTextWidthUTF8(label) : 8.f * (float)label.size();
            float th = ctx.font ? (ctx.font->Ascent() - ctx.font->Descent()) : 18.f;
            if (Transform().size.x <= 0) Transform().size.x = box + 6.f + tw + m_Style.padding * 2.f;
            if (Transform().size.y <= 0) Transform().size.y = std::max(box, th) + m_Style.padding * 2.f;
        }

        bool OnPointer(const UIPointerEvent& ev) override
        {
            if (!IsEnabled()) return false;
            if (ev.down && HitTest(ev.x, ev.y)) { checked = !checked; return true; }
            return false;
        }

        bool OnKey(const UIKeyEvent& ev) override
        {
            if (!IsEnabled() || !IsFocused()) return false;
            const int KEY_SPACE = 32, KEY_ENTER = 257;
            if (ev.down && (ev.key == KEY_SPACE || ev.key == KEY_ENTER)) { checked = !checked; return true; }
            return false;
        }

        void BuildDraw(UIRenderContext& ctx) override
        {
            Rect r = Transform().rect;
            float th = ctx.defaultFont ? (ctx.defaultFont->Ascent() - ctx.defaultFont->Descent()) : 18.f;
            float box = 16.f;
            Rect boxR{ r.x + m_Style.padding, r.y + (r.h - box) / 2.f, box, box };
            ctx.batcher->PushRect(boxR, ctx.whiteTex, PackRGBA8({ 0.2f,0.22f,0.25f,1 }), nullptr);
            if (checked) {
                Rect in{ boxR.x + 3, boxR.y + 3, boxR.w - 6, boxR.h - 6 };
                ctx.batcher->PushRect(in, ctx.whiteTex, PackRGBA8({ 0.1f,0.6f,0.2f,1 }), nullptr);
            }
            float lx = boxR.x + box + 6.f;
            float ly = r.y + (r.h - th) * 0.5f;
            ctx.CtxDrawText(label.c_str(), lx, ly, m_Style.fg);
        }

    private:
        void Init() { m_Style.padding = 6.f; SetFocusable(true); }
    };

    class UIRadioButton : public UIElement
    {
    public:
        using UIElement::UIElement;
        std::string label = "Radio";
        int index = 0;
        int* groupValue = nullptr;

        explicit UIRadioButton(std::string id) : UIElement(std::move(id)) { Init(); }
        UIRadioButton() : UIElement("radio") { Init(); }

        bool selected() const { return groupValue && *groupValue == index; }

        void Measure(UILayoutContext& ctx) override
        {
            float d = 16.f;
            float tw = ctx.font ? ctx.font->MeasureTextWidthUTF8(label) : 8.f * (float)label.size();
            float th = ctx.font ? (ctx.font->Ascent() - ctx.font->Descent()) : 18.f;
            if (Transform().size.x <= 0) Transform().size.x = d + 6.f + tw + m_Style.padding * 2.f;
            if (Transform().size.y <= 0) Transform().size.y = std::max(d, th) + m_Style.padding * 2.f;
        }

        bool OnPointer(const UIPointerEvent& ev) override
        {
            if (!IsEnabled() || !groupValue) return false;
            if (ev.down && HitTest(ev.x, ev.y)) { *groupValue = index; return true; }
            return false;
        }
        bool OnKey(const UIKeyEvent& ev) override
        {
            if (!IsEnabled() || !IsFocused() || !groupValue) return false;
            const int KEY_SPACE = 32, KEY_ENTER = 257;
            if (ev.down && (ev.key == KEY_SPACE || ev.key == KEY_ENTER)) { *groupValue = index; return true; }
            return false;
        }

        void BuildDraw(UIRenderContext& ctx) override
        {
            Rect r = Transform().rect;
            float d = 16.f;
            Rect circ{ r.x + m_Style.padding, r.y + (r.h - d) / 2.f, d, d };
            ctx.batcher->PushRect(circ, ctx.whiteTex, PackRGBA8({ 0.2f,0.22f,0.25f,1 }), nullptr);
            if (selected()) {
                Rect in{ circ.x + 4, circ.y + 4, circ.w - 8, circ.h - 8 };
                ctx.batcher->PushRect(in, ctx.whiteTex, PackRGBA8({ 0.8f,0.8f,0.2f,1 }), nullptr);
            }
            float th = ctx.defaultFont ? (ctx.defaultFont->Ascent() - ctx.defaultFont->Descent()) : 18.f;
            float lx = circ.x + d + 6.f;
            float ly = r.y + (r.h - th) * 0.5f;
            ctx.CtxDrawText(label.c_str(), lx, ly, m_Style.fg);
        }

    private:
        void Init() { m_Style.padding = 6.f; SetFocusable(true); }
    };
}
