#include "qepch.h"

#include "UIDebug.h"
#include "UIInput.h"

namespace QuasarEngine {
    static bool IsTab(int key) { return key == 9; }
    static bool IsShift(int key) { return key == 340 || key == 344; }

    static UIElement* HitTestDFS(UIElement* node, float x, float y) {
        auto& kids = node->Children();
        for (int i = (int)kids.size() - 1; i >= 0; --i) {
            auto* got = HitTestDFS(kids[i].get(), x, y);
            if (got) return got;
        }
        if (node->HitTest(x, y)) return node;
        return nullptr;
    }

    UIElement* UIInput::dispatchPointer(UIElement* node, const UIPointerEvent& ev) {
        UIElement* target = m_Captured ? m_Captured : HitTestDFS(node, ev.x, ev.y);
        if (!target) return nullptr;
        if (!target->IsEnabled()) return nullptr;

        if (ev.down) { m_Captured = target; if (target->IsFocusable()) RequestFocus(target); }
        target->OnPointer(ev);
        if (ev.up) m_Captured = nullptr;
        return target;
    }

    UIElement* UIInput::dispatchKey(UIElement* node, const UIKeyEvent& ev) {
        if (node->OnKey(ev)) return node;
        for (auto& c : node->Children()) {
            if (auto* got = dispatchKey(c.get(), ev)) return got;
        }
        return nullptr;
    }

    UIElement* UIInput::dispatchChar(UIElement* node, const UICharEvent& ev)
    {
        if (node->OnChar(ev.codepoint)) return node;
        for (auto& c : node->Children()) {
            if (auto* got = dispatchChar(c.get(), ev)) return got;
        }
        return nullptr;
    }

    void UIInput::collectFocusables(UIElement* node, std::vector<UIElement*>& out) {
        if (!node->IsEnabled()) return;

        const Rect& r = node->Transform().rect;
        const bool hasArea = (r.w > 0.f && r.h > 0.f);

        if (node->IsFocusable() && hasArea) out.push_back(node);
        for (auto& c : node->Children()) collectFocusables(c.get(), out);
    }

    void UIInput::FeedPointer(const UIPointerEvent& ev) {
        if (auto root = m_Root.lock()) {
            dispatchPointer(root.get(), ev);
        }
        else {
            UI_DIAG_WARN("UIInput: root expired; event dropped.");
        }
    }

    void UIInput::FeedKey(const UIKeyEvent& ev) {
        auto root = m_Root.lock(); if (!root) return;

        if (ev.down && IsTab(ev.key)) {
            FocusNext();
            return;
        }

        if (m_Focused && m_Focused->OnKey(ev)) return;

        dispatchKey(root.get(), ev);
    }

    void UIInput::FeedChar(const UICharEvent& ev)
    {
        auto root = m_Root.lock(); if (!root) return;
        if (m_Focused && m_Focused->OnChar(ev.codepoint)) return;
        dispatchChar(root.get(), ev);
    }

    void UIInput::RequestFocus(UIElement* el)
    {
        if (m_Focused == el) return;
        if (m_Focused) m_Focused->OnBlur();
        m_Focused = el;
        if (m_Focused) m_Focused->OnFocus();
    }

    void UIInput::FocusNext()
    {
        auto root = m_Root.lock(); if (!root) return;
        std::vector<UIElement*> list; list.reserve(64);
        collectFocusables(root.get(), list);
        if (list.empty()) return;
        std::sort(list.begin(), list.end(), [](auto* a, auto* b) { return a->GetTabIndex() < b->GetTabIndex(); });
        if (!m_Focused) { RequestFocus(list.front()); return; }
        auto it = std::find(list.begin(), list.end(), m_Focused);
        if (it == list.end() || ++it == list.end()) RequestFocus(list.front());
        else RequestFocus(*it);
    }

    void UIInput::FocusPrev()
    {
        auto root = m_Root.lock(); if (!root) return;
        std::vector<UIElement*> list; list.reserve(64);
        collectFocusables(root.get(), list);
        if (list.empty()) return;
        std::sort(list.begin(), list.end(), [](auto* a, auto* b) { return a->GetTabIndex() < b->GetTabIndex(); });
        if (!m_Focused) { RequestFocus(list.back()); return; }
        auto it = std::find(list.begin(), list.end(), m_Focused);
        if (it == list.begin() || it == list.end()) RequestFocus(list.back());
        else { --it; RequestFocus(*it); }
    }
}