#include "qepch.h"

#include "DirectXBuffer.h"

#include "Platform/DirectX/DirectXContext.h"

#include <QuasarEngine/Core/Logger.h>

namespace QuasarEngine
{
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
			Q_ERROR("DirectXUniformBuffer: CreateBuffer failed.");
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
			Q_ERROR("DirectXUniformBuffer: Map failed.");
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
		: m_Size(size), m_IsDynamic(true)
	{
		auto& dx = DirectXContext::Context;

		D3D11_BUFFER_DESC desc = {};
		desc.ByteWidth = static_cast<UINT>(size);
		desc.Usage = D3D11_USAGE_DYNAMIC;
		desc.BindFlags = D3D11_BIND_VERTEX_BUFFER;
		desc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
		desc.MiscFlags = 0;

		HRESULT hr = dx.device->CreateBuffer(&desc, nullptr, m_Buffer.GetAddressOf());
		if (FAILED(hr))
		{
			Q_ERROR("DirectXVertexBuffer: CreateBuffer(size) failed.");
		}
	}

	DirectXVertexBuffer::DirectXVertexBuffer(const std::vector<float>& vertices)
		: m_Size(vertices.size() * sizeof(float)), m_IsDynamic(true)
	{
		auto& dx = DirectXContext::Context;

		D3D11_BUFFER_DESC desc = {};
		desc.ByteWidth = static_cast<UINT>(m_Size);
		desc.Usage = D3D11_USAGE_DYNAMIC;
		desc.BindFlags = D3D11_BIND_VERTEX_BUFFER;
		desc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
		desc.MiscFlags = 0;

		D3D11_SUBRESOURCE_DATA init = {};
		init.pSysMem = vertices.data();

		HRESULT hr = dx.device->CreateBuffer(&desc, &init, m_Buffer.GetAddressOf());
		if (FAILED(hr))
		{
			Q_ERROR("DirectXVertexBuffer: CreateBuffer(init) failed.");
		}
	}

	DirectXVertexBuffer::~DirectXVertexBuffer()
	{
		
	}

	void DirectXVertexBuffer::Bind() const
	{
		auto& dx = DirectXContext::Context;
		UINT stride = m_Layout.GetStride();
		UINT offset = 0;

		dx.deviceContext->IASetVertexBuffers(0, 1, m_Buffer.GetAddressOf(), &stride, &offset);
	}

	void DirectXVertexBuffer::Unbind() const
	{
		auto& dx = DirectXContext::Context;
		ID3D11Buffer* nullBuf = nullptr;
		UINT stride = 0, offset = 0;
		dx.deviceContext->IASetVertexBuffers(0, 1, &nullBuf, &stride, &offset);
	}

	void DirectXVertexBuffer::UploadVertices(const std::vector<float> vertices)
	{
		auto& dx = DirectXContext::Context;
		size_t bytes = vertices.size() * sizeof(float);

		if (!m_Buffer || bytes > m_Size)
		{
			m_Size = bytes;

			D3D11_BUFFER_DESC desc = {};
			desc.ByteWidth = static_cast<UINT>(m_Size);
			desc.Usage = D3D11_USAGE_DYNAMIC;
			desc.BindFlags = D3D11_BIND_VERTEX_BUFFER;
			desc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;

			D3D11_SUBRESOURCE_DATA init = {};
			init.pSysMem = vertices.data();

			HRESULT hr = dx.device->CreateBuffer(&desc, &init, m_Buffer.GetAddressOf());
			if (FAILED(hr))
			{
				Q_ERROR("DirectXVertexBuffer: UploadVertices CreateBuffer failed.");
			}
			return;
		}

		D3D11_MAPPED_SUBRESOURCE mapped = {};
		HRESULT hr = dx.deviceContext->Map(m_Buffer.Get(), 0, D3D11_MAP_WRITE_DISCARD, 0, &mapped);
		if (FAILED(hr))
		{
			Q_ERROR("DirectXVertexBuffer: Map failed.");
			return;
		}
		memcpy(mapped.pData, vertices.data(), bytes);
		dx.deviceContext->Unmap(m_Buffer.Get(), 0);
	}

	DirectXIndexBuffer::DirectXIndexBuffer()
		: m_Count(0), m_IsDynamic(true)
	{
		
	}

	DirectXIndexBuffer::DirectXIndexBuffer(const std::vector<uint32_t> indices)
		: m_Count(indices.size()), m_IsDynamic(true)
	{
		auto& dx = DirectXContext::Context;

		D3D11_BUFFER_DESC desc = {};
		desc.ByteWidth = static_cast<UINT>(indices.size() * sizeof(uint32_t));
		desc.Usage = D3D11_USAGE_DYNAMIC;
		desc.BindFlags = D3D11_BIND_INDEX_BUFFER;
		desc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;

		D3D11_SUBRESOURCE_DATA init = {};
		init.pSysMem = indices.data();

		HRESULT hr = dx.device->CreateBuffer(&desc, &init, m_Buffer.GetAddressOf());
		if (FAILED(hr))
		{
			Q_ERROR("DirectXIndexBuffer: CreateBuffer(init) failed.");
		}
	}

	DirectXIndexBuffer::~DirectXIndexBuffer()
	{
		
	}

	void DirectXIndexBuffer::Bind() const
	{
		auto& dx = DirectXContext::Context;
		dx.deviceContext->IASetIndexBuffer(m_Buffer.Get(), DXGI_FORMAT_R32_UINT, 0);
	}

	void DirectXIndexBuffer::Unbind() const
	{
		auto& dx = DirectXContext::Context;
		dx.deviceContext->IASetIndexBuffer(nullptr, DXGI_FORMAT_R32_UINT, 0);
	}

	void DirectXIndexBuffer::UploadIndices(const std::vector<uint32_t> indices)
	{
		auto& dx = DirectXContext::Context;
		m_Count = indices.size();
		size_t bytes = indices.size() * sizeof(uint32_t);

		if (!m_Buffer)
		{
			D3D11_BUFFER_DESC desc = {};
			desc.ByteWidth = static_cast<UINT>(bytes);
			desc.Usage = D3D11_USAGE_DYNAMIC;
			desc.BindFlags = D3D11_BIND_INDEX_BUFFER;
			desc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;

			D3D11_SUBRESOURCE_DATA init = {};
			init.pSysMem = indices.data();

			HRESULT hr = dx.device->CreateBuffer(&desc, &init, m_Buffer.GetAddressOf());
			if (FAILED(hr))
			{
				Q_ERROR("DirectXIndexBuffer: UploadIndices CreateBuffer failed.");
			}
			return;
		}

		D3D11_BUFFER_DESC currentDesc = {};
		m_Buffer->GetDesc(&currentDesc);
		if (bytes > currentDesc.ByteWidth)
		{
			D3D11_BUFFER_DESC desc = {};
			desc.ByteWidth = static_cast<UINT>(bytes);
			desc.Usage = D3D11_USAGE_DYNAMIC;
			desc.BindFlags = D3D11_BIND_INDEX_BUFFER;
			desc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;

			D3D11_SUBRESOURCE_DATA init = {};
			init.pSysMem = indices.data();

			m_Buffer.Reset();
			HRESULT hr = dx.device->CreateBuffer(&desc, &init, m_Buffer.GetAddressOf());
			if (FAILED(hr))
			{
				Q_ERROR("DirectXIndexBuffer: UploadIndices resize CreateBuffer failed.");
			}
			return;
		}

		D3D11_MAPPED_SUBRESOURCE mapped = {};
		HRESULT hr = dx.deviceContext->Map(m_Buffer.Get(), 0, D3D11_MAP_WRITE_DISCARD, 0, &mapped);
		if (FAILED(hr))
		{
			Q_ERROR("DirectXIndexBuffer: Map failed.");
			return;
		}
		memcpy(mapped.pData, indices.data(), bytes);
		dx.deviceContext->Unmap(m_Buffer.Get(), 0);
	}
}