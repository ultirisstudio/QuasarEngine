#include "qepch.h"

#include "DirectXBuffer.h"

#include "Platform/DirectX/DirectXContext.h"

#include <QuasarEngine/Core/Logger.h>

namespace QuasarEngine
{
	static inline UINT NextPow2(UINT v) {
		if (v < 1024u) return 1024u;
		v--; v |= v >> 1; v |= v >> 2; v |= v >> 4; v |= v >> 8; v |= v >> 16; v++;
		return v;
	}

	DirectXUniformBuffer::DirectXUniformBuffer(size_t size, uint32_t binding)
		: m_Size(size), m_Binding(binding)
	{
		auto& dx = DirectXContext::Context;

		D3D11_BUFFER_DESC desc = {};
		desc.ByteWidth = static_cast<UINT>((size + 15) & ~15u);
		desc.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
		desc.Usage = D3D11_USAGE_DYNAMIC;
		desc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
		desc.MiscFlags = 0;

		HRESULT hr = dx.device->CreateBuffer(&desc, nullptr, m_Buffer.GetAddressOf());
		if (FAILED(hr))
		{
			Q_ERROR("CreateBuffer failed.");
		}
	}

	DirectXUniformBuffer::~DirectXUniformBuffer()
	{
		
	}

	void DirectXUniformBuffer::SetData(const void* data, size_t size)
	{
		if (size > m_Size)
		{
			throw std::runtime_error(
				"Uniform buffer size exceeded: requested " + std::to_string(size) +
				" bytes, but buffer size is " + std::to_string(m_Size) + " bytes."
			);
		}
		if (size != m_Size)
		{
			std::cerr << "[Warning] Constant buffer size mismatch: expected "
				<< m_Size << " but got " << size << " bytes.\n";
		}

		auto& dx = DirectXContext::Context;

		D3D11_MAPPED_SUBRESOURCE mapped = {};
		HRESULT hr = dx.deviceContext->Map(m_Buffer.Get(), 0, D3D11_MAP_WRITE_DISCARD, 0, &mapped);
		if (FAILED(hr))
		{
			Q_ERROR("Map failed.");
			return;
		}
		memcpy(mapped.pData, data, size);
		dx.deviceContext->Unmap(m_Buffer.Get(), 0);

		dx.deviceContext->VSSetConstantBuffers(m_Binding, 1, m_Buffer.GetAddressOf());
		dx.deviceContext->PSSetConstantBuffers(m_Binding, 1, m_Buffer.GetAddressOf());
	}

	DirectXVertexBuffer::DirectXVertexBuffer()
		: m_Size(0), m_IsDynamic(true)
	{
	}

	DirectXVertexBuffer::DirectXVertexBuffer(uint32_t size)
		: m_Size(0), m_IsDynamic(true)
	{
		Reserve(size);
	}

	DirectXVertexBuffer::DirectXVertexBuffer(const void* data, uint32_t size)
		: m_Size(0), m_IsDynamic(true)
	{
		Reserve(size);
		Upload(data, size);
	}

	DirectXVertexBuffer::~DirectXVertexBuffer()
	{
		
	}

	void DirectXVertexBuffer::Bind() const {
		auto& dx = DirectXContext::Context;
		UINT stride = m_Layout.GetStride();
		UINT offset = 0;
		ID3D11Buffer* buf = m_Buffer.Get();
		dx.deviceContext->IASetVertexBuffers(0, 1, &buf, &stride, &offset);
	}

	void DirectXVertexBuffer::Unbind() const {
		auto& dx = DirectXContext::Context;
		ID3D11Buffer* nullBuf = nullptr;
		UINT stride = 0, offset = 0;
		dx.deviceContext->IASetVertexBuffers(0, 1, &nullBuf, &stride, &offset);
	}

	void DirectXVertexBuffer::Reserve(uint32_t size) {
		auto& dx = DirectXContext::Context;
		if (size <= m_Size && m_Buffer) return;

		const UINT cap = NextPow2(size);

		D3D11_BUFFER_DESC desc = {};
		desc.ByteWidth = cap;
		desc.Usage = D3D11_USAGE_DYNAMIC;
		desc.BindFlags = D3D11_BIND_VERTEX_BUFFER;
		desc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;

		m_Buffer.Reset();
		HRESULT hr = dx.device->CreateBuffer(&desc, nullptr, m_Buffer.GetAddressOf());
		if (FAILED(hr)) {
			Q_ERROR("CreateBuffer failed (capacity): " + std::to_string(cap));
			return;
		}

		m_Size = cap;
	}

	void DirectXVertexBuffer::Upload(const void* data, uint32_t size) {
		if (!data || size == 0) return;

		auto& dx = DirectXContext::Context;
		if (!m_Buffer || size > m_Size) {
			Reserve(size);
		}

		D3D11_MAPPED_SUBRESOURCE mapped = {};
		HRESULT hr = dx.deviceContext->Map(m_Buffer.Get(), 0, D3D11_MAP_WRITE_DISCARD, 0, &mapped);
		if (FAILED(hr)) {
			Q_ERROR("Map failed.");
			return;
		}
		std::memcpy(mapped.pData, data, size);
		dx.deviceContext->Unmap(m_Buffer.Get(), 0);
	}

	DirectXIndexBuffer::DirectXIndexBuffer()
		: m_Size(0), m_IsDynamic(true)
	{
		
	}

	DirectXIndexBuffer::DirectXIndexBuffer(uint32_t size)
		: m_Size(0), m_IsDynamic(true)
	{
		Reserve(size);
	}

	DirectXIndexBuffer::DirectXIndexBuffer(const void* data, uint32_t size)
		: m_Size(0), m_IsDynamic(true)
	{
		Reserve(size);
		Upload(data, size);
	}

	DirectXIndexBuffer::~DirectXIndexBuffer()
	{
		
	}

	void DirectXIndexBuffer::Bind() const {
		auto& dx = DirectXContext::Context;
		dx.deviceContext->IASetIndexBuffer(m_Buffer.Get(), DXGI_FORMAT_R32_UINT, 0);
	}

	void DirectXIndexBuffer::Unbind() const {
		auto& dx = DirectXContext::Context;
		dx.deviceContext->IASetIndexBuffer(nullptr, DXGI_FORMAT_R32_UINT, 0);
	}

	void DirectXIndexBuffer::Reserve(uint32_t size) {
		auto& dx = DirectXContext::Context;
		if (size <= m_Size && m_Buffer) return;

		const UINT cap = NextPow2(size);

		D3D11_BUFFER_DESC desc = {};
		desc.ByteWidth = cap;
		desc.Usage = D3D11_USAGE_DYNAMIC;
		desc.BindFlags = D3D11_BIND_INDEX_BUFFER;
		desc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;

		m_Buffer.Reset();
		HRESULT hr = dx.device->CreateBuffer(&desc, nullptr, m_Buffer.GetAddressOf());
		if (FAILED(hr)) {
			Q_ERROR("CreateBuffer failed (capacity): " + std::to_string(cap));
			return;
		}

		m_Size = cap;
	}

	void DirectXIndexBuffer::Upload(const void* data, uint32_t size) {
		if (!data || size == 0) return;

		auto& dx = DirectXContext::Context;
		if (!m_Buffer || size > m_Size) {
			Reserve(size);
		}

		D3D11_MAPPED_SUBRESOURCE mapped = {};
		HRESULT hr = dx.deviceContext->Map(m_Buffer.Get(), 0, D3D11_MAP_WRITE_DISCARD, 0, &mapped);
		if (FAILED(hr)) {
			Q_ERROR("Map failed.");
			return;
		}
		std::memcpy(mapped.pData, data, size);
		dx.deviceContext->Unmap(m_Buffer.Get(), 0);
	}
}