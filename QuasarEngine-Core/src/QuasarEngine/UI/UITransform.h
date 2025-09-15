#pragma once

#include <cstdint>

namespace QuasarEngine
{
	struct Vec2 {
		float x, y;
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