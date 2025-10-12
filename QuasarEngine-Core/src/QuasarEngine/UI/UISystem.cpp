#include "qepch.h"

#include "UIDebug.h"
#include "UISystem.h"
#include "UIContainer.h"

#include <glm/gtc/matrix_transform.hpp>

#include <QuasarEngine/Renderer/Renderer.h>
#include <QuasarEngine/Asset/AssetManager.h>
#include <QuasarEngine/Resources/Texture2D.h>

namespace QuasarEngine {
	void UISystem::MeasureLayout(const UIFBInfo& fb) {
		if (!m_Root) return;
		UILayoutContext lctx{};
		lctx.dpiScale = fb.dpiScale;
		lctx.font = m_Renderer.Ctx().defaultFont;
		m_Root->Measure(lctx);
	}

	void UISystem::Arrange(const UIFBInfo& fb) {
		if (!m_Root) return;
		Rect rootRect{ 0.f, 0.f, (float)fb.width, (float)fb.height };
		m_Root->Arrange(rootRect);
	}

	UISystem::UISystem() : m_Root(nullptr), m_Input(), m_Renderer()
	{
		
	}

	void UISystem::Render(BaseCamera& camera, const UIFBInfo& fb) {
		if (!m_Root) {
			UI_DIAG_WARN("UISystem: no root set; skipping UI render.");
			return;
		}
		
		MeasureLayout(fb);
		Rect rootRect{ 0.f, 0.f, (float)fb.width, (float)fb.height };
		m_Root->Arrange(rootRect);

		if (!m_Renderer.GetShader()) {
			UI_DIAG_ERROR("UISystem: UI shader missing.");
			return;
		}

		m_Renderer.GetShader()->Use();

		glm::mat4 proj = glm::ortho(0.0f, (float)fb.width, (float)fb.height, 0.0f, -1.0f, 1.0f);
		glm::mat4 model = glm::mat4(1.0f);

		m_Renderer.GetShader()->SetUniform("projection", &proj, sizeof(glm::mat4));
		m_Renderer.GetShader()->SetUniform("model", &model, sizeof(glm::mat4));

		if (!m_Renderer.GetShader()->UpdateGlobalState()) {
			UI_DIAG_ERROR("UISystem: UpdateGlobalState() failed for UI shader.");
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