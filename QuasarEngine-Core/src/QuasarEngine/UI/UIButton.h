#pragma once

#include "UIElement.h"

#include <functional>

namespace QuasarEngine {
	class UIButton : public UIElement {
	public:
		using UIElement::UIElement;
		std::string label = "Button";
		std::function<void()> onClick;
		bool hovered = false, pressed = false, enabled = true;

		bool OnPointer(const UIPointerEvent& ev) override;
		void Measure(UILayoutContext& ctx) override;
		void BuildDraw(UIRenderContext& ctx) override;
	};
}