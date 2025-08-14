#pragma once

#include "qepch.h"
#include "QuasarEngine/Events/Event.h"
#include "QuasarEngine/Renderer/GraphicsContext.h"
#include <glm/glm.hpp>

struct GLFWwindow;

namespace QuasarEngine {
	
	class Window
	{
	public:
		using EventCallbackFn = std::function<void(Event&)>;

		struct WindowData
		{
			std::string Title;
			unsigned int Width, Height;
			bool VSync;
			glm::uvec2 MousePos;

			EventCallbackFn EventCallback;

			WindowData(const std::string& title = "Quasar Engine", uint32_t width = 1280, uint32_t height = 720, bool vsync = false)
				: Title(title), Width(width), Height(height), VSync(vsync), MousePos(glm::uvec2(0.0f, 0.0f))
			{}
		};

		Window();
		virtual ~Window();

		void PollEvents();

		void BeginFrame();
		void EndFrame();

		void Resize(unsigned int width, unsigned int height);

		inline unsigned int GetWidth() const { return m_Data.Width; }
		inline unsigned int GetHeight() const { return m_Data.Height; }

		template<typename T>
		T* GetContextAs() const {
			return dynamic_cast<T*>(m_Context.get());
		}

		void SetVSync(bool enabled);
		void SetInputMode(bool cursorDisabled, bool rawMouseMotion);
		bool IsVSync() const;

		void SetCursorVisibility(bool visible);

		inline virtual void* GetNativeWindow() const { return m_Window; }

		void SetTitle(const std::string& title);

		inline void SetEventCallback(const EventCallbackFn& callback) { m_Data.EventCallback = callback; }
	private:
		virtual void Shutdown();
	private:
		GLFWwindow* m_Window;
		WindowData m_Data;

		std::unique_ptr<GraphicsContext> m_Context;
	};

}