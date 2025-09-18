#include "qepch.h"

#include "UISystem.h"
#include "UIContainer.h"

#include <glm/gtc/matrix_transform.hpp>

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

		glm::mat4 proj = glm::ortho(0.0f, (float)fb.width, (float)fb.height, 0.0f, -1.0f, 1.0f);
		glm::mat4 model = glm::mat4(1.0f);

		m_Renderer.GetShader()->SetUniform("proj", &proj, sizeof(glm::mat4));
		m_Renderer.GetShader()->SetUniform("model", &model, sizeof(glm::mat4));

		if (!m_Renderer.GetShader()->UpdateGlobalState())
		{
			return;
		}

		if (!m_Renderer.GetShader()->UpdateObject(&m_Renderer.GetMaterial()))
		{
			return;
		}

		m_Renderer.Begin(fb.width, fb.height);
		m_Renderer.Ctx().dpiScale = fb.dpiScale;
		m_Root->BuildDraw(m_Renderer.Ctx());
		m_Renderer.End();

		m_Renderer.GetShader()->Reset();

		m_Renderer.GetShader()->Unuse();
	}
}