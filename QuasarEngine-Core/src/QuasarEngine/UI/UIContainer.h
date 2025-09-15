#pragma once

#include "UIElement.h"

namespace QuasarEngine {
	enum class UILayoutType {
		Vertical,
		Horizontal
	};

	class UIContainer : public UIElement {
	public:
		using UIElement::UIElement;
		UILayoutType layout = UILayoutType::Vertical;
		float gap = 6.f;

		void Measure(UILayoutContext& ctx) override;
		void Arrange(const Rect& parentRect) override;
		void BuildDraw(UIRenderContext& ctx) override;
	};
}