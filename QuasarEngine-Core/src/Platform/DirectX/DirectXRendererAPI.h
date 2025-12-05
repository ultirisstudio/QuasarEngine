#pragma once

#include "QuasarEngine/Renderer/RendererAPI.h"

namespace QuasarEngine {

	class DirectXRendererAPI : public RendererAPI
	{
	public:
		void Initialize() override;
		void Shutdown() override {}

		void SetViewport(uint32_t x, uint32_t y, uint32_t width, uint32_t height) override;

		void ClearColor(const glm::vec4& color) override;
		void Clear() override;

		void SetSeamlessCubemap(bool enable) override {}

		void DrawArrays(DrawMode drawMode, uint32_t size) override;
		void DrawArraysInstanced(DrawMode drawMode, uint32_t size, uint32_t instanceCount) override;
		void DrawElements(DrawMode drawMode, uint32_t count, uint32_t firstIndex, int32_t baseVertex) override;
		void DrawElementsInstanced(DrawMode drawMode, uint32_t count, uint32_t instanceCount, uint32_t firstIndex, int32_t baseVertex) override;

		void EnableScissor(bool enable) override {}
		void SetScissorRect(uint32_t x, uint32_t y, uint32_t width, uint32_t height) override {}
	};
}
