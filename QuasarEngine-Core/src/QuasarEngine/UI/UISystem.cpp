#include "qepch.h"

#include "UISystem.h"
#include "UIContainer.h"

#include <glm/gtc/matrix_transform.hpp>

#include <QuasarEngine/Resources/Materials/Material.h>

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

	void UISystem::Render(BaseCamera& camera, const UIFBInfo& fb) {
		if (!m_Root) return;
		
		MeasureLayout();
		Rect rootRect{ 0.f, 0.f, (float)fb.width, (float)fb.height };
		m_Root->Arrange(rootRect);

		m_Renderer.GetShader()->Use();

		glm::mat4 uProj = glm::ortho(0.0f, (float)fb.width, (float)fb.height, 0.0f, -1.0f, 1.0f);

		m_Renderer.GetShader()->SetUniform("uProj", &uProj, sizeof(glm::mat4));

		if (!m_Renderer.GetShader()->UpdateGlobalState())
		{
			return;
		}

		MaterialSpecification spec;
		Material material = Material(spec);

		if (!m_Renderer.GetShader()->UpdateObject(&material))
		{
			return;
		}

		m_Renderer.Begin(fb.width, fb.height);
		m_Renderer.Ctx().dpiScale = fb.dpiScale;
		m_Root->BuildDraw(m_Renderer.Ctx());
		m_Renderer.End();

		m_Renderer.GetShader()->Reset();
	}
}