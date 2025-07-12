#include "qepch.h"
#include "QuasarEngine/Core/Input.h"

#include "QuasarEngine/Core/Application.h"
#include <GLFW/glfw3.h>

#include <unordered_map>

namespace QuasarEngine {
	std::unordered_map<KeyCode, bool> Input::s_KeyStates;
	std::unordered_map<KeyCode, bool> Input::s_KeyPressedThisFrame;

	bool Input::IsKeyPressed(const KeyCode key)
	{
		return s_KeyStates[key];
	}

	bool Input::IsKeyJustPressed(KeyCode key)
	{
		return s_KeyPressedThisFrame[key];
	}

	bool Input::IsMouseButtonPressed(const MouseCode button)
	{
		auto* window = static_cast<GLFWwindow*>(Application::Get().GetWindow().GetNativeWindow());
		auto state = glfwGetMouseButton(window, static_cast<int32_t>(button));
		return state == GLFW_PRESS;
	}

	glm::vec2 Input::GetMousePosition()
	{
		auto* window = static_cast<GLFWwindow*>(Application::Get().GetWindow().GetNativeWindow());
		double xpos, ypos;
		glfwGetCursorPos(window, &xpos, &ypos);

		return { (float)xpos, (float)ypos };
	}

	float Input::GetMouseX()
	{
		return GetMousePosition().x;
	}

	float Input::GetMouseY()
	{
		return GetMousePosition().y;
	}

	void Input::Update()
	{
		auto* window = static_cast<GLFWwindow*>(Application::Get().GetWindow().GetNativeWindow());

		for (int key = GLFW_KEY_SPACE; key <= GLFW_KEY_LAST; key++)
		{
			int state = glfwGetKey(window, key);
			bool isDown = state == GLFW_PRESS;

			if (isDown && !s_KeyStates[key])
				s_KeyPressedThisFrame[key] = true;
			else
				s_KeyPressedThisFrame[key] = false;

			s_KeyStates[key] = isDown;
		}
	}
}