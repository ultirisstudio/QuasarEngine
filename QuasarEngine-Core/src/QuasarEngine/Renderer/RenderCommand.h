#pragma once

#include <QuasarEngine/Core/Singleton.h>
#include <QuasarEngine/Renderer/RendererAPI.h>

namespace QuasarEngine
{
	class RenderCommand : public Singleton<RenderCommand>
	{
	public:
		void Initialize()
		{
			s_RendererAPI = RendererAPI::Create();

			s_RendererAPI->Initialize();
		}

		void Shutdown()
		{
			s_RendererAPI->Shutdown();
		}

		void SetViewport(uint32_t x, uint32_t y, uint32_t width, uint32_t height)
		{
			s_RendererAPI->SetViewport(x, y, width, height);
		}

		void ClearColor(const glm::vec4& color)
		{
			s_RendererAPI->ClearColor(color);
		}

		void Clear()
		{
			s_RendererAPI->Clear();
		}

		void SetSeamlessCubemap(bool enable)
		{
			s_RendererAPI->SetSeamlessCubemap(enable);
		}

void DrawArrays(DrawMode drawMode, uint32_t size)
{
s_RendererAPI->DrawArrays(drawMode, size);
}

void DrawArraysInstanced(DrawMode drawMode, uint32_t size, uint32_t instanceCount)
{
s_RendererAPI->DrawArraysInstanced(drawMode, size, instanceCount);
}

void DrawElements(DrawMode drawMode, uint32_t count, uint32_t firstIndex = 0, int32_t baseVertex = 0)
{
s_RendererAPI->DrawElements(drawMode, count, firstIndex, baseVertex);
}

void DrawElementsInstanced(DrawMode drawMode, uint32_t count, uint32_t instanceCount, uint32_t firstIndex = 0, int32_t baseVertex = 0)
{
s_RendererAPI->DrawElementsInstanced(drawMode, count, instanceCount, firstIndex, baseVertex);
}

		void EnableScissor(bool enable)
		{
			s_RendererAPI->EnableScissor(enable);
		}

		void SetScissor(uint32_t x, uint32_t y, uint32_t width, uint32_t height)
		{
			s_RendererAPI->SetScissorRect(x, y, width, height);
		}

	private:
		std::unique_ptr<RendererAPI> s_RendererAPI;
	};
}