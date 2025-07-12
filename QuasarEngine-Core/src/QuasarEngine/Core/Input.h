#pragma once

#include "KeyCodes.h"
#include "MouseCodes.h"

#include <glm/glm.hpp>

namespace QuasarEngine {
	class Input
	{
	public:
		static bool IsKeyPressed(KeyCode key);
		static bool IsKeyJustPressed(KeyCode key);

		static bool IsMouseButtonPressed(MouseCode button);
		static glm::vec2 GetMousePosition();

		static float GetMouseX();
		static float GetMouseY();

		static void Update();

	private:
		static std::unordered_map<KeyCode, bool> s_KeyStates;
		static std::unordered_map<KeyCode, bool> s_KeyPressedThisFrame;
	};
}