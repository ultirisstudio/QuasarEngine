#pragma once

#include "UIElement.h"
#include "UITransform.h"
#include "UIRenderer.h"

namespace QuasarEngine {

    class UITextInput : public UIElement {
    public:
        using UIElement::UIElement;
        std::string text;
        size_t caret = 0;
        size_t selStart = 0, selEnd = 0;
        bool readOnly = false;
        float minWidth = 120.f;

        explicit UITextInput(std::string id) : UIElement(std::move(id)) { Init(); }
        UITextInput() : UIElement("textinput") { Init(); }

        void Measure(UILayoutContext& ctx) override {
            float th = ctx.font ? (ctx.font->Ascent() - ctx.font->Descent()) : 18.f;
            if (Transform().size.y <= 0) Transform().size.y = th + m_Style.padding * 2.f;
            if (Transform().size.x <= 0) {
                float w = ctx.font ? ctx.font->MeasureTextWidthUTF8(text) : 8.f * (float)text.size();
                Transform().size.x = std::max(minWidth, w + m_Style.padding * 2.f);
            }
        }

        bool OnPointer(const UIPointerEvent& ev) override {
            if (readOnly) return false;
            if (ev.down || ev.move) {
                if (HitTest(ev.x, ev.y)) {
                    size_t newCaret = hitTestToIndex(ev.x);
                    if (ev.down) { caret = newCaret; selStart = selEnd = caret; m_Selecting = true; }
                    else if (m_Selecting) { selEnd = newCaret; }
                    return true;
                }
            }
            if (ev.up) m_Selecting = false;
            return false;
        }

        bool OnKey(const UIKeyEvent& ev) override {
            if (!IsFocused()) return false;
            if (!ev.down) return false;

            const int KEY_LEFT = 263, KEY_RIGHT = 262, KEY_BACKSPACE = 259, KEY_DELETE = 261, KEY_HOME = 268, KEY_END = 269, KEY_A = 65, KEY_C = 67, KEY_V = 86, KEY_X = 88, KEY_ENTER = 257;

            bool ctrl = false;
            bool shift = false;

            switch (ev.key) {
            case KEY_LEFT:  moveCaret(-1, shift); return true;
            case KEY_RIGHT: moveCaret(+1, shift); return true;
            case KEY_HOME:  setCaret(0, shift);   return true;
            case KEY_END:   setCaret(utf8_len(), shift); return true;
            case KEY_BACKSPACE: if (!readOnly) backspace(); return true;
            case KEY_DELETE:    if (!readOnly) del();       return true;
            case KEY_ENTER:     /* commit/notify */ return true;
            case KEY_A: if (ctrl) { selStart = 0; selEnd = utf8_len(); caret = selEnd; return true; } break;
            case KEY_C: if (ctrl) { /* TODO: clipboard copy selection */ return true; } break;
            case KEY_V: if (ctrl && !readOnly) { /* TODO: paste */ return true; } break;
            case KEY_X: if (ctrl && !readOnly) { /* TODO: cut */ return true; } break;
            default: break;
            }
            return false;
        }

        bool OnChar(uint32_t cp) override {
            if (!IsFocused() || readOnly) return false;
            if (cp < 32) return false;
            replaceSelectionWith(cp_to_utf8(cp));
            return true;
        }

        void BuildDraw(UIRenderContext& ctx) override {
            auto r = Transform().rect;
            UIColor bg = m_Style.bg;
            if (!IsEnabled()) bg.a *= 0.5f;
            ctx.batcher->PushRect(r, ctx.whiteTex, PackRGBA8(bg), nullptr);

            if (hasSelection()) {
                auto seg = selectionRects(ctx);
                for (auto& s : seg) {
                    ctx.batcher->PushRect(s, ctx.whiteTex, PackRGBA8({ 0.2f,0.4f,0.8f,0.35f }), nullptr);
                }
            }

            float tx = r.x + m_Style.padding;
            float ty = r.y + m_Style.padding;
            ctx.DrawText(text.c_str(), tx, ty, m_Style.fg);

            if (IsFocused() && m_CaretBlinkOn) {
                Rect rc = caretRect(ctx);
                ctx.batcher->PushRect(rc, ctx.whiteTex, PackRGBA8({ 1,1,1,1 }), nullptr);
            }

            m_BlinkCounter++;
            if (m_BlinkCounter > 30) { m_CaretBlinkOn = !m_CaretBlinkOn; m_BlinkCounter = 0; }
        }

    private:
        bool m_Selecting = false;
        bool m_CaretBlinkOn = true;
        int  m_BlinkCounter = 0;

        bool hasSelection() const { return selStart != selEnd; }
        size_t selMin() const { return std::min(selStart, selEnd); }
        size_t selMax() const { return std::max(selStart, selEnd); }

        size_t utf8_len() const { return text.size(); }

        void clearSelection() { selStart = selEnd = caret; }

        void setCaret(size_t pos, bool keepSelection) {
            caret = std::min(pos, utf8_len());
            if (!keepSelection) selStart = selEnd = caret;
            m_BlinkCounter = 0; m_CaretBlinkOn = true;
        }

        void moveCaret(int dir, bool keepSelection) {
            if (dir < 0 && caret > 0) setCaret(caret - 1, keepSelection);
            else if (dir > 0 && caret < utf8_len()) setCaret(caret + 1, keepSelection);
        }

        void backspace() {
            if (hasSelection()) { deleteSelection(); return; }
            if (caret == 0) return;
            text.erase(caret - 1, 1); caret--; clearSelection();
        }
        void del() {
            if (hasSelection()) { deleteSelection(); return; }
            if (caret >= utf8_len()) return;
            text.erase(caret, 1); clearSelection();
        }
        void deleteSelection() {
            auto a = selMin(), b = selMax();
            text.erase(a, b - a);
            caret = a; clearSelection();
        }

        void replaceSelectionWith(const std::string& ins) {
            if (hasSelection()) deleteSelection();
            text.insert(caret, ins);
            caret += ins.size();
            clearSelection();
        }

        std::string cp_to_utf8(uint32_t cp) {
            char buf[5] = { 0 };
            if (cp < 0x80) { buf[0] = (char)cp; return std::string(buf, 1); }
            if (cp < 0x800) { buf[0] = 0xC0 | (cp >> 6); buf[1] = 0x80 | (cp & 0x3F); return std::string(buf, 2); }
            if (cp < 0x10000) { buf[0] = 0xE0 | (cp >> 12); buf[1] = 0x80 | ((cp >> 6) & 0x3F); buf[2] = 0x80 | (cp & 0x3F); return std::string(buf, 3); }
            buf[0] = 0xF0 | (cp >> 18); buf[1] = 0x80 | ((cp >> 12) & 0x3F); buf[2] = 0x80 | ((cp >> 6) & 0x3F); buf[3] = 0x80 | (cp & 0x3F); return std::string(buf, 4);
        }

        float measureX(UIRenderContext& ctx, size_t i) const {
            auto* f = ctx.defaultFont;
            if (!f) return (float)i * 8.f;
            return f->MeasureTextWidthUTF8(text.substr(0, i));
        }

        size_t hitTestToIndex(float px) const {
            float localX = px - (Transform().rect.x + m_Style.padding);
            if (localX <= 0) return 0;
            size_t i = 0;
            float acc = 0.f;
            float avg = text.empty() ? 8.f : (Transform().size.x - 2 * m_Style.padding) / std::max(1.0f, (float)text.size());
            i = std::min((size_t)std::floor(localX / std::max(1.f, avg)), text.size());
            return i;
        }

        Rect caretRect(UIRenderContext& ctx) const {
            float x = Transform().rect.x + m_Style.padding + measureX(ctx, caret);
            float h = Transform().size.y - 2 * m_Style.padding;
            return { x, Transform().rect.y + m_Style.padding, 1.0f, h };
        }

        void Init()
        {
            SetFocusable(true);
            m_Style.padding = 6.f;
        }

        std::vector<Rect> selectionRects(UIRenderContext& ctx) const {
            std::vector<Rect> out;
            auto a = selMin(), b = selMax();
            float x0 = Transform().rect.x + m_Style.padding + measureX(ctx, a);
            float x1 = Transform().rect.x + m_Style.padding + measureX(ctx, b);
            float h = Transform().size.y - 2 * m_Style.padding;
            out.push_back({ x0, Transform().rect.y + m_Style.padding, x1 - x0, h });
            return out;
        }
    };
}