#pragma once

#include <QuasarEngine/Renderer/Buffer.h>

#include <wrl/client.h>
#include <d3d11.h>

namespace QuasarEngine
{
	class DirectXUniformBuffer
	{
	public:
		DirectXUniformBuffer(size_t size, uint32_t binding);
		~DirectXUniformBuffer();

		void SetData(const void* data, size_t size);

		ID3D11Buffer* GetBuffer() const { return m_Buffer.Get(); }
		size_t GetSize() const { return m_Size; }
		uint32_t GetBinding() const { return m_Binding; }

	private:
		Microsoft::WRL::ComPtr<ID3D11Buffer> m_Buffer;
		size_t m_Size = 0;
		uint32_t m_Binding = 0;
	};

	class DirectXVertexBuffer : public VertexBuffer
	{
	public:
		DirectXVertexBuffer();
		DirectXVertexBuffer(uint32_t size);
		DirectXVertexBuffer(const void* data, uint32_t size);

		~DirectXVertexBuffer() override;

		void Bind() const override;
		void Unbind() const override;

		void Upload(const void* data, uint32_t size) override;

		void Reserve(uint32_t size) override;

		size_t GetSize() const override { return m_Size; }

		const BufferLayout& GetLayout() const override { return m_Layout; }
		void SetLayout(const BufferLayout& layout) override { m_Layout = layout; }

	private:
		Microsoft::WRL::ComPtr<ID3D11Buffer> m_Buffer;
		size_t m_Size = 0;
		BufferLayout m_Layout;
		bool m_IsDynamic = true;
	};

	class DirectXIndexBuffer : public IndexBuffer
	{
	public:
		DirectXIndexBuffer();
		DirectXIndexBuffer(uint32_t size);
		DirectXIndexBuffer(const void* data, uint32_t size);

		~DirectXIndexBuffer() override;

		void Bind() const override;
		void Unbind() const override;

		void Upload(const void* data, uint32_t size) override;

		void Reserve(uint32_t size) override;

		size_t GetSize() const override { return m_Size; }
		uint32_t GetCount() const override { return (uint32_t)(m_Size / sizeof(uint32_t)); }

	private:
		Microsoft::WRL::ComPtr<ID3D11Buffer> m_Buffer;
		size_t m_Size = 0;
		bool m_IsDynamic = true;
	};
}