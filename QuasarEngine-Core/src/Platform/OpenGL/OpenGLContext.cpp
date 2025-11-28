#include "qepch.h"
#include "OpenGLContext.h"

#include <GLFW/glfw3.h>
#include <glad/glad.h>

#include "QuasarEngine/Core/Logger.h"

namespace QuasarEngine {

	OpenGLContext::OpenGLContext(GLFWwindow* windowHandle)
		: m_WindowHandle(windowHandle)
	{
		glfwMakeContextCurrent(m_WindowHandle);
		int status = gladLoadGLLoader((GLADloadproc)glfwGetProcAddress);
		if (!status)
			Q_ERROR("Failed to initialize GLAD");

		Q_INFO("OpenGL Info:");
		Q_INFO((char*)glGetString(GL_VENDOR));
		Q_INFO((char*)glGetString(GL_RENDERER));
		Q_INFO((char*)glGetString(GL_VERSION));

		int maj = 0, min = 0;
		glGetIntegerv(GL_MAJOR_VERSION, &maj);
		glGetIntegerv(GL_MINOR_VERSION, &min);
		Q_INFO("OpenGL version: " + std::to_string(maj) + "." + std::to_string(min));
		if (maj < 4 || (maj == 4 && min < 5)) {
			Q_WARNING("OpenGL < 4.5 detected: ARB_direct_state_access may be required, DSA code paths could fail.");
		}

		glEnable(GL_PROGRAM_POINT_SIZE);
	}

	void OpenGLContext::BeginFrame()
	{
	}

	void OpenGLContext::EndFrame()
	{
		glfwSwapBuffers(m_WindowHandle);
	}

	void OpenGLContext::Resize(unsigned int newWidth, unsigned int newHeight)
	{
		glViewport(0, 0, (GLsizei)newWidth, (GLsizei)newHeight);
	}
}