#pragma once

#include "QuasarEngine/Renderer/GraphicsContext.h"

struct GLFWwindow;

namespace QuasarEngine {

	class OpenGLContext : public GraphicsContext
	{
	public:
		OpenGLContext(GLFWwindow* windowHandle);

		void BeginFrame() override;
		void EndFrame() override;

		void Resize(unsigned int newWidth, unsigned int newHeight) override;
	private:
		GLFWwindow* m_WindowHandle;
	};

}