#pragma once

#include "UIElement.h"

namespace QuasarEngine {
	class UIText : public UIElement {
	public:
		using UIElement::UIElement;
		std::string text;
		float fontSize = 18.f;
		void Measure(UILayoutContext& ctx) override; 
		void BuildDraw(UIRenderContext& ctx) override;
	};
}