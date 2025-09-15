#pragma once

#include <cstdint>

namespace QuasarEngine
{
	struct UIColor {
		float r = 1.f, g = 1.f, b = 1.f, a = 1.f;
	};

	inline uint32_t PackRGBA8(const UIColor& c) {
		auto clamp = [&](float v) { if (v < 0) v = 0; if (v > 1) v = 1; return (uint32_t)(v * 255.f + 0.5f); };
		return (clamp(c.a) << 24) | (clamp(c.b) << 16) | (clamp(c.g) << 8) | (clamp(c.r));
	}

	struct UIStyle {
		float padding = 8.f;
		float margin = 0.f;
		float radius = 8.f;

		UIColor bg{ 0.10f,0.11f,0.12f,1.f };
		UIColor fg{ 0.95f,0.96f,0.98f,1.f };

		bool dirtyVisual = true;
	};
}