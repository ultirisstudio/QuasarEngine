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
	}
}