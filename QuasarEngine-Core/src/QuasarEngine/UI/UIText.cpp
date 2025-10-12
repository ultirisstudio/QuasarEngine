#include "qepch.h"

#include "UIDebug.h"
#include "UIText.h"
#include "UIRenderer.h"

namespace QuasarEngine {
    void UIText::Measure(UILayoutContext& ctx) {
        float textW = 0.f, textH = 0.f;

        if (ctx.font) {
            textW = ctx.font->MeasureTextWidthUTF8(text);
            textH = ctx.font->Ascent() - ctx.font->Descent();
            // textH = ctx.font->LineHeight();
        }
        else {
            UI_DIAG_WARN("UIText: no font in layout context; using fallback metrics.");

            textW = 8.f * (float)text.size();
            textH = 18.f;
        }

        if (Transform().size.x <= 0) Transform().size.x = textW;
        if (Transform().size.y <= 0) Transform().size.y = textH;
    }

	void UIText::BuildDraw(UIRenderContext& ctx) {
		ctx.DrawText(text.c_str(), Transform().rect.x, Transform().rect.y, m_Style.fg);
	}
}