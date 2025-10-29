#pragma once

#include <string>
#include <vector>
#include <memory>
#include <functional>

#include "UITransform.h"
#include "UIStyle.h"
#include "UIFont.h"

namespace QuasarEngine
{
	struct UIPointerEvent
	{
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


	struct UICharEvent {
		uint32_t codepoint = 0;
	};

	struct UIRenderContext;

	struct UILayoutContext {
		float dpiScale = 1.f;
		UIFont* font = nullptr;
	};

	enum class HAlign { Left, Center, Right };
	enum class VAlign { Top, Middle, Baseline, Bottom };

	class UIElement : public std::enable_shared_from_this<UIElement> {
		using ChildList = std::vector<std::shared_ptr<UIElement>>;
	public:
		explicit UIElement(std::string id) : m_Id(std::move(id)) {}
		virtual ~UIElement() = default;

		void AddChild(std::shared_ptr<UIElement> c) { m_Children.push_back(std::move(c)); }

		const ChildList& Children() const { return m_Children; }
		ChildList& Children() { return m_Children; }

		virtual void Measure(UILayoutContext& ctx);
		virtual void Arrange(const Rect& parentRect);
		virtual void BuildDraw(UIRenderContext& ctx);

		virtual bool HitTest(float x, float y) const { return m_Transform.rect.Contains(x, y); }
		virtual bool OnPointer(const UIPointerEvent& ev) { return false; }
		virtual bool OnKey(const UIKeyEvent& ev) { return false; }

		virtual bool OnChar(uint32_t codepoint) { return false; }

		virtual void OnFocus() { m_Focused = true; }
		virtual void OnBlur() { m_Focused = false; }

		bool IsFocusable() const { return m_Focusable && m_Enabled; }
		void SetFocusable(bool f) { m_Focusable = f; }
		void SetTabIndex(int i) { m_TabIndex = i; }
		int  GetTabIndex() const { return m_TabIndex; }
		bool IsFocused() const { return m_Focused; }

		void SetEnabled(bool e) { m_Enabled = e; }
		bool IsEnabled() const { return m_Enabled; }

		void SetTextAlign(HAlign h, VAlign v) { m_TextAlignH = h; m_TextAlignV = v; }
		HAlign GetTextAlignH() const { return m_TextAlignH; }
		VAlign GetTextAlignV() const { return m_TextAlignV; }

		UITransform& Transform() { return m_Transform; }
		UIStyle& Style() { return m_Style; }

		const UITransform& Transform() const { return m_Transform; }

		const UIStyle& Style()     const { return m_Style; }

		const std::string& Id() const { return m_Id; }
		void SetId(const std::string& id) { m_Id = id; }

	protected:
		std::string m_Id;
		UITransform m_Transform;
		UIStyle     m_Style;

		bool m_Enabled = true;
		bool m_Focusable = false;
		bool m_Focused = false;
		int  m_TabIndex = 0;
		HAlign m_TextAlignH = HAlign::Left;
		VAlign m_TextAlignV = VAlign::Top;

		ChildList m_Children;
	};
}