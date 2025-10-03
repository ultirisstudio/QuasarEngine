#pragma once

#include <memory>

#include "UIElement.h"
#include "UIInput.h"
#include "UIRenderer.h"

#include <QuasarEngine/Scene/BaseCamera.h>

namespace QuasarEngine {
	struct UIFBInfo {
		int width = 1280, height = 720;
		float dpiScale = 1.f;
	};

	class UISystem {
	public:
		UISystem();
		~UISystem() = default;

		void SetRoot(std::shared_ptr<UIElement> root) { m_Root = std::move(root); m_Input.SetRoot(m_Root); }
		void Render(BaseCamera& camera, const UIFBInfo& fb);

		UIInput& Input() { return m_Input; }
		UIRenderer& Renderer() { return m_Renderer; }

	private:
		void MeasureLayout();
		void Arrange(const UIFBInfo& fb);

		std::shared_ptr<UIElement> m_Root;
		UIInput m_Input;
		UIRenderer m_Renderer;
	};
}