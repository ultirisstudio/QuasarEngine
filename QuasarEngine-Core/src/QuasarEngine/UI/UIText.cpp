#include "qepch.h"

#include "UIText.h"
#include "UIRenderer.h"

namespace QuasarEngine {
	void UIText::Measure(UILayoutContext&) {
		float textW = 8.f * (float)text.size();
		float textH = 18.f;
		if (Transform().size.x <= 0) Transform().size.x = textW;
		if (Transform().size.y <= 0) Transform().size.y = textH;
	}

	void UIText::BuildDraw(UIRenderContext& ctx) {
		ctx.DrawDebugText(text.c_str(), Transform().rect.x, Transform().rect.y, m_Style.fg);
	}
}