#pragma once

#include <memory>

#include "UIElement.h"

namespace QuasarEngine {
	class UIInput {
	public:
		void SetRoot(const std::shared_ptr<UIElement>& root) { m_Root = root; }
		void FeedPointer(const UIPointerEvent& ev);
		void FeedKey(const UIKeyEvent& ev);

	private:
		std::weak_ptr<UIElement> m_Root;
		UIElement* dispatchPointer(UIElement* node, const UIPointerEvent& ev);
		UIElement* dispatchKey(UIElement* node, const UIKeyEvent& ev);
	};
}