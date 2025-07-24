#pragma once

#include <memory>
#include <glm/glm.hpp>

#include <QuasarEngine/Renderer/DrawMode.h>

namespace QuasarEngine {

	class RendererAPI
	{
	public:
		enum class API
		{
			None = 0,
			Vulkan,
			OpenGL
		};
	public:
		virtual ~RendererAPI() = default;

		virtual void Init() = 0;
		virtual void SetViewport(uint32_t x, uint32_t y, uint32_t width, uint32_t height) = 0;
		virtual void ClearColor(const glm::vec4& color) = 0;
		virtual void Clear() = 0;

		virtual void DrawArrays(DrawMode drawMode, uint32_t size) = 0;
		virtual void DrawElements(DrawMode drawMode, uint32_t count) = 0;

		static API GetAPI() { return s_API; }
		static std::unique_ptr<RendererAPI> Create();
	private:
		static API s_API;
	};

}