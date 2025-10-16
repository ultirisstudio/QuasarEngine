#pragma once

#include <memory>

#include "UIElement.h"

namespace QuasarEngine
{
	class UIInput {
	public:
		void SetRoot(const std::shared_ptr<UIElement>& root) { m_Root = root; }

		void FeedPointer(const UIPointerEvent& ev);
		void FeedKey(const UIKeyEvent& ev);
		void FeedChar(const UICharEvent& ev);

		void RequestFocus(UIElement* el);
		void FocusNext();
		void FocusPrev();

	private:
		std::weak_ptr<UIElement> m_Root;

		UIElement* m_Captured = nullptr;
		UIElement* m_Focused = nullptr;

		UIElement* dispatchPointer(UIElement* node, const UIPointerEvent& ev);
		UIElement* dispatchKey(UIElement* node, const UIKeyEvent& ev);
		UIElement* dispatchChar(UIElement* node, const UICharEvent& ev);

		void collectFocusables(UIElement* node, std::vector<UIElement*>& out);
	};
}