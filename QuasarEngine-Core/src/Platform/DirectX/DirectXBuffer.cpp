#include "qepch.h"

#include "DirectXBuffer.h"

namespace QuasarEngine
{
	DirectXUniformBuffer::DirectXUniformBuffer(size_t size, uint32_t binding)
		: m_Size(size), m_Binding(binding)
	{

	}

	DirectXUniformBuffer::~DirectXUniformBuffer()
	{
		
	}

	void DirectXUniformBuffer::SetData(const void* data, size_t size)
	{
		if (size > m_Size)
			throw std::runtime_error("Uniform buffer size exceeded: requested " + std::to_string(size) + " bytes, but buffer size is " + std::to_string(m_Size) + " bytes.");

		if (size != m_Size)
		{
			std::cerr << "[Warning] Uniform buffer size mismatch: expected " << m_Size << " bytes but got " << size << " bytes." << std::endl;
		}
	}

	void DirectXUniformBuffer::BindToShader(uint32_t programID, const std::string& blockName)
	{
		
	}

	uint32_t DirectXUniformBuffer::GetID() const
	{
		return m_ID;
	}

	DirectXVertexBuffer::DirectXVertexBuffer()
	{
	}

	DirectXVertexBuffer::DirectXVertexBuffer(uint32_t size)
		: m_Size(size)
	{
		
	}

	DirectXVertexBuffer::DirectXVertexBuffer(const std::vector<float>& vertices)
		: m_Size(vertices.size())
	{
		
	}

	DirectXVertexBuffer::~DirectXVertexBuffer()
	{
		
	}

	void DirectXVertexBuffer::Bind() const
	{
		
	}

	void DirectXVertexBuffer::Unbind() const
	{
		
	}

	void DirectXVertexBuffer::UploadVertices(const std::vector<float> vertices)
	{
		
	}

	DirectXIndexBuffer::DirectXIndexBuffer()
	{
	}

	DirectXIndexBuffer::DirectXIndexBuffer(const std::vector<uint32_t> indices)
		: m_Count(indices.size())
	{
		
	}

	DirectXIndexBuffer::~DirectXIndexBuffer()
	{
		
	}

	void DirectXIndexBuffer::Bind() const
	{
		
	}

	void DirectXIndexBuffer::Unbind() const
	{
		
	}

	void DirectXIndexBuffer::UploadIndices(const std::vector<uint32_t> indices)
	{
		
	}
}