#pragma once

#include <memory>

namespace QuasarEngine {

	class GraphicsContext
	{
	public:
		virtual ~GraphicsContext() = default;

		virtual void BeginFrame() = 0;
		virtual void EndFrame() = 0;
		virtual void Resize(unsigned int width, unsigned int height) = 0;

		static std::unique_ptr<GraphicsContext> Create(void* window);
	};
}