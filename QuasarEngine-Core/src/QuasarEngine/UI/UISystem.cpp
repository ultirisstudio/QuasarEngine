#include "qepch.h"

#include "UISystem.h"
#include "UIContainer.h"

namespace QuasarEngine {
	void UISystem::MeasureLayout() {
		if (!m_Root) return;
		UILayoutContext lctx{};
		m_Root->Measure(lctx);
	}

	void UISystem::Arrange(const UIFBInfo& fb) {
		if (!m_Root) return;
		Rect rootRect{ 0.f, 0.f, (float)fb.width, (float)fb.height };
		m_Root->Arrange(rootRect);
	}

	void UISystem::Render() {
		if (!m_Root) return;
		auto& r = m_Renderer;
		r.Begin((int)r.Ctx().dpiScale, (int)r.Ctx().dpiScale);
	}

	void UISystem::Tick(float dt, const UIFBInfo& fb) {
		if (!m_Root) return;

		MeasureLayout();
		Rect rootRect{ 0.f, 0.f, (float)fb.width, (float)fb.height };
		m_Root->Arrange(rootRect);

		m_Renderer.Begin(fb.width, fb.height);
		m_Renderer.Ctx().dpiScale = fb.dpiScale;
		m_Root->BuildDraw(m_Renderer.Ctx());
		m_Renderer.End();
	}
}