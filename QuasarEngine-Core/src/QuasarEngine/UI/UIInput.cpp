#include "qepch.h"

#include "UIInput.h"

namespace QuasarEngine {
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
        UIElement* target = HitTestDFS(node, ev.x, ev.y);
        if (target) target->OnPointer(ev);
        return target;
    }
    UIElement* UIInput::dispatchKey(UIElement* node, const UIKeyEvent& ev) {
        if (node->OnKey(ev)) return node;
        for (auto& c : node->Children()) {
            if (auto* got = dispatchKey(c.get(), ev)) return got;
        }
        return nullptr;
    }

    void UIInput::FeedPointer(const UIPointerEvent& ev) {
        if (auto root = m_Root.lock()) dispatchPointer(root.get(), ev);
    }
    void UIInput::FeedKey(const UIKeyEvent& ev) {
        if (auto root = m_Root.lock()) dispatchKey(root.get(), ev);
    }
}