#include "qepch.h"
#include "VulkanVertexArray.h"

#include "VulkanBuffer.h"

namespace QuasarEngine
{
	VulkanVertexArray::VulkanVertexArray()
	{
		
	}

	VulkanVertexArray::~VulkanVertexArray()
	{
		for (auto& vb : m_VertexBuffers)
		{
			vb.reset();
		}

		if (m_IndexBuffer)
		{
			m_IndexBuffer.reset();
		}
	}

	void VulkanVertexArray::Bind() const
	{
		for (auto& vb : m_VertexBuffers)
		{
			vb->Bind();
		}

		if (m_IndexBuffer)
		{
			m_IndexBuffer->Bind();
		}
	}

	void VulkanVertexArray::Unbind() const
	{
		
	}

	void VulkanVertexArray::AddVertexBuffer(const std::shared_ptr<VertexBuffer>& vertexBuffer)
	{
		m_VertexBuffers.push_back(vertexBuffer);
	}

	void VulkanVertexArray::SetIndexBuffer(const std::shared_ptr<IndexBuffer>& indexBuffer)
	{
		m_IndexBuffer = indexBuffer;
	}
}