#include "qepch.h"
#include "OpenGLRendererAPI.h"

#include <iostream>
#include <string>
#include <glad/glad.h>
#include <GLFW/glfw3.h>

#include "QuasarEngine/Renderer/DrawMode.h"

namespace QuasarEngine {

	namespace Utils
	{
		static GLenum DrawModeToGLenum(DrawMode drawMode)
		{
			switch (drawMode)
			{
			case DrawMode::TRIANGLES: return GL_TRIANGLES;
			case DrawMode::TRIANGLE_STRIP: return GL_TRIANGLE_STRIP;
			case DrawMode::TRIANGLE_FAN: return GL_TRIANGLE_FAN;
			case DrawMode::LINES: return GL_LINES;
			case DrawMode::LINE_STRIP: return GL_LINE_STRIP;
			case DrawMode::LINE_LOOP: return GL_LINE_LOOP;
			case DrawMode::POINTS: return GL_POINTS;
			}
			return GL_TRIANGLES;
		}
	}

	void OpenGLMessageCallback(
		unsigned source,
		unsigned type,
		unsigned id,
		unsigned severity,
		int length,
		const char* message,
		const void* userParam)
	{
		switch (severity)
		{
		case GL_DEBUG_SEVERITY_HIGH:         std::cout << std::string(message) << std::endl; return;
		case GL_DEBUG_SEVERITY_MEDIUM:       std::cout << std::string(message) << std::endl; return;
		case GL_DEBUG_SEVERITY_LOW:          std::cout << std::string(message) << std::endl; return;
		case GL_DEBUG_SEVERITY_NOTIFICATION: std::cout << std::string(message) << std::endl; return;
		}

	}

	void OpenGLRendererAPI::Init()
	{
#ifdef DEBUG
		glEnable(GL_DEBUG_OUTPUT);
		glEnable(GL_DEBUG_OUTPUT_SYNCHRONOUS);
		glDebugMessageCallback(OpenGLMessageCallback, nullptr);

		glDebugMessageControl(GL_DONT_CARE, GL_DONT_CARE, GL_DEBUG_SEVERITY_NOTIFICATION, 0, NULL, GL_FALSE);
#endif
		glEnable(GL_BLEND);
		glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

		glCullFace(GL_BACK);

		glEnable(GL_DEPTH_TEST);
		glDepthFunc(GL_LEQUAL);
		glEnable(GL_LINE_SMOOTH);
	}

	void OpenGLRendererAPI::SetViewport(uint32_t x, uint32_t y, uint32_t width, uint32_t height)
	{
		glViewport(x, y, width, height);
	}

	void OpenGLRendererAPI::ClearColor(const glm::vec4& color)
	{
		glClearColor(color.r, color.g, color.b, color.a);
	}

	void OpenGLRendererAPI::Clear()
	{
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	}

	void OpenGLRendererAPI::SetDepthTest(bool enabled)
	{
		if (enabled)
			glEnable(GL_DEPTH_TEST);
		else
			glDisable(GL_DEPTH_TEST);
	}

	void OpenGLRendererAPI::DrawArrays(DrawMode drawMode, uint32_t size)
	{
		glDrawArrays(Utils::DrawModeToGLenum(drawMode), 0, static_cast<GLsizei>(size));
	}

	void OpenGLRendererAPI::DrawElements(DrawMode drawMode, uint32_t count)
	{
		glDrawElements(Utils::DrawModeToGLenum(drawMode), static_cast<GLsizei>(count), GL_UNSIGNED_INT, nullptr);
	}
}