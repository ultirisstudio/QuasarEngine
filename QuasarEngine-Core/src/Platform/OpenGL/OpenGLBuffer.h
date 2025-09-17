#pragma once

#include <QuasarEngine/Renderer/Buffer.h>

namespace QuasarEngine
{
	class OpenGLUniformBuffer
	{
	public:
		OpenGLUniformBuffer(size_t size, uint32_t binding);
		~OpenGLUniformBuffer();

		void SetData(const void* data, size_t size);
		void BindToShader(uint32_t programID, const std::string& blockName);

		uint32_t GetID() const;

	private:
		uint32_t m_ID = 0;
		size_t m_Size = 0;
		uint32_t m_Binding = 0;
	};

	class OpenGLVertexBuffer : public VertexBuffer
	{
	public:
		OpenGLVertexBuffer();
		OpenGLVertexBuffer(uint32_t size);
		OpenGLVertexBuffer(const void* data, uint32_t size);

		~OpenGLVertexBuffer() override;

		void Bind() const override;
		void Unbind() const override;

		void Upload(const void* data, uint32_t size) override;

		void Reserve(uint32_t size) override;

		size_t GetSize() const override { return m_Size; }

		const BufferLayout& GetLayout() const override { return m_Layout; }
		void SetLayout(const BufferLayout& layout) override { m_Layout = layout; }
	private:
		uint32_t m_RendererID;
		size_t m_Size;
		BufferLayout m_Layout;
	};

	class OpenGLIndexBuffer : public IndexBuffer
	{
	public:
		OpenGLIndexBuffer();
		OpenGLIndexBuffer(uint32_t size);
		OpenGLIndexBuffer(const void* data, uint32_t size);

		~OpenGLIndexBuffer() override;

		void Bind() const override;
		void Unbind() const override;

		void Upload(const void* data, uint32_t size) override;

		void Reserve(uint32_t size) override;

		size_t GetSize() const override { return m_Size; }
		uint32_t GetCount() const override { return m_Size / sizeof(uint32_t); }
	private:
		uint32_t m_RendererID;
		size_t m_Size;
	};
}