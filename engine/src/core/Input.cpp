#include <quasar/core/Application.h>
#include <quasar/core/Input.h>

#include <GLFW/glfw3.h>

namespace Quasar::Input
{
    namespace
    {
        GLFWwindow* GetMainWindow()
        {
            return Application::Get().GetWindow().GetNativeWindow();
        }
    }

    bool IsKeyPressed(Key key)
    {
        return glfwGetKey(GetMainWindow(), static_cast<int>(key)) == GLFW_PRESS;
    }

    bool IsMouseButtonPressed(MouseButton button)
    {
        return glfwGetMouseButton(GetMainWindow(), static_cast<int>(button)) == GLFW_PRESS;
    }

    glm::vec2 GetMousePosition()
    {
        double x = 0.0;
        double y = 0.0;
        glfwGetCursorPos(GetMainWindow(), &x, &y);
        return {static_cast<float>(x), static_cast<float>(y)};
    }
}
