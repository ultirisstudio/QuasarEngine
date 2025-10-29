#pragma once

#include <cstdint>

#include <glm/glm.hpp>

namespace QuasarEngine
{
	struct Vec2 {
		float x{ 0.f }, y{ 0.f };

		Vec2() = default;
		Vec2(float x_, float y_) : x(x_), y(y_) {}
		Vec2(const glm::vec2& v) : x(v.x), y(v.y) {}

		Vec2& operator=(const glm::vec2& v) { x = v.x; y = v.y; return *this; }

		Vec2& operator+=(const Vec2& o) { x += o.x; y += o.y; return *this; }
		Vec2& operator+=(const glm::vec2& o) { x += o.x; y += o.y; return *this; }
	};

	struct Rect {
		float x = 0.f, y = 0.f, w = 0.f, h = 0.f;
		inline bool Contains(float px, float py) const {
			return px >= x && py >= y && px <= x + w && py <= y + h;
		}
	};

	struct UIAnchors {
		float minX = 0, minY = 0, maxX = 0, maxY = 0;
	};

	struct UIPivot {
		float x = 0.5f, y = 0.5f;
	};

	struct UITransform {
		Vec2 pos{ 0,0 };
		Vec2 size{ 0,0 };
		UIAnchors anchors{};
		UIPivot  pivot{};
		Rect     rect{};
		bool     dirtyLayout = true;
	};
}