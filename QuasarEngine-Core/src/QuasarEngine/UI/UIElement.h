#pragma once

#include <string>
#include <vector>
#include <memory>
#include <functional>

#include "UITransform.h"
#include "UIStyle.h"
#include "UIFont.h"

namespace QuasarEngine {
	struct UIPointerEvent {
		float x, y;
		bool down = false;
		bool up = false;
		bool move = false;
		int button = 0;
	};

	struct UIKeyEvent {
		int key = 0;
		bool down = false;
	};

	struct UIRenderContext;

	struct UILayoutContext {
		float dpiScale = 1.f;
		UIFont* font = nullptr;
	};

	class UIElement {
	public:
		explicit UIElement(std::string id) : m_Id(std::move(id)) {}
		virtual ~UIElement() = default;

		void AddChild(std::shared_ptr<UIElement> c) { m_Children.push_back(std::move(c)); }
		const std::vector<std::shared_ptr<UIElement>>& Children() const { return m_Children; }

		virtual void Measure(UILayoutContext& ctx);
		virtual void Arrange(const Rect& parentRect);
		virtual void BuildDraw(UIRenderContext& ctx);

		virtual bool HitTest(float x, float y) const { return m_Transform.rect.Contains(x, y); }
		virtual bool OnPointer(const UIPointerEvent& ev) { return false; }
		virtual bool OnKey(const UIKeyEvent& ev) { return false; }

		UITransform& Transform() { return m_Transform; }
		UIStyle& Style() { return m_Style; }
		const std::string& Id() const { return m_Id; }

	protected:
		std::string m_Id;
		UITransform m_Transform;
		UIStyle     m_Style;

		std::vector<std::shared_ptr<UIElement>> m_Children;
	};
}