#pragma once

#include <QuasarEngine/Renderer/Buffer.h>

namespace QuasarEngine
{
	class DirectXUniformBuffer
	{
	public:
		DirectXUniformBuffer(size_t size, uint32_t binding);
		~DirectXUniformBuffer();

		void SetData(const void* data, size_t size);
		void BindToShader(uint32_t programID, const std::string& blockName);

		uint32_t GetID() const;

	private:
		uint32_t m_ID = 0;
		size_t m_Size = 0;
		uint32_t m_Binding = 0;
	};

	class DirectXVertexBuffer : public VertexBuffer
	{
	public:
		DirectXVertexBuffer();
		DirectXVertexBuffer(uint32_t size);
		DirectXVertexBuffer(const std::vector<float>& vertices);
		virtual ~DirectXVertexBuffer();

		virtual void Bind() const override;
		virtual void Unbind() const override;

		void UploadVertices(const std::vector<float> vertices) override;

		size_t GetSize() const override { return m_Size; }

		virtual const BufferLayout& GetLayout() const override { return m_Layout; }
		virtual void SetLayout(const BufferLayout& layout) override { m_Layout = layout; }
	private:
		uint32_t m_RendererID;
		size_t m_Size;
		BufferLayout m_Layout;
	};

	class DirectXIndexBuffer : public IndexBuffer
	{
	public:
		DirectXIndexBuffer();
		DirectXIndexBuffer(const std::vector<uint32_t> indices);
		virtual ~DirectXIndexBuffer();

		virtual void Bind() const;
		virtual void Unbind() const;

		void UploadIndices(const std::vector<uint32_t> indices) override;

		virtual size_t GetCount() const { return m_Count; }
	private:
		uint32_t m_RendererID;
		size_t m_Count;
	};
}