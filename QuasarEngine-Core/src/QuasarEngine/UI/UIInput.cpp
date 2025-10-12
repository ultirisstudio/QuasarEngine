#include "qepch.h"

#include "UIDebug.h"
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
        UIElement* target = m_Captured ? m_Captured : HitTestDFS(node, ev.x, ev.y);

        if (!target) return nullptr;

        if (ev.down) m_Captured = target;

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

    void UIInput::FeedPointer(const UIPointerEvent& ev) {
        if (auto root = m_Root.lock()) {
            dispatchPointer(root.get(), ev);
        }
        else {
            UI_DIAG_WARN("UIInput: root expired; event dropped.");
        }
    }

    void UIInput::FeedKey(const UIKeyEvent& ev) {
        if (auto root = m_Root.lock()) dispatchKey(root.get(), ev);
    }
}