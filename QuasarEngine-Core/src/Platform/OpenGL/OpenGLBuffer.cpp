#include "qepch.h"
#include "OpenGLBuffer.h"

#include <glad/glad.h>

namespace QuasarEngine
{
	OpenGLUniformBuffer::OpenGLUniformBuffer(size_t size, uint32_t binding)
		: m_Size(size), m_Binding(binding)
	{
		glGenBuffers(1, &m_ID);
		glBindBuffer(GL_UNIFORM_BUFFER, m_ID);
		glBufferData(GL_UNIFORM_BUFFER, size, nullptr, GL_DYNAMIC_DRAW);
		glBindBufferBase(GL_UNIFORM_BUFFER, binding, m_ID);
		glBindBuffer(GL_UNIFORM_BUFFER, 0);
	}

	OpenGLUniformBuffer::~OpenGLUniformBuffer()
	{
		glDeleteBuffers(1, &m_ID);
	}

	void OpenGLUniformBuffer::SetData(const void* data, size_t size)
	{
		if (size > m_Size)
			throw std::runtime_error("Uniform buffer size exceeded: requested " + std::to_string(size) + " bytes, but buffer size is " + std::to_string(m_Size) + " bytes.");

		if (size != m_Size)
		{
			std::cerr << "[Warning] Uniform buffer size mismatch: expected " << m_Size << " bytes but got " << size << " bytes." << std::endl;
		}

		glBindBuffer(GL_UNIFORM_BUFFER, m_ID);
		glBufferSubData(GL_UNIFORM_BUFFER, 0, size, data);
		glBindBufferBase(GL_UNIFORM_BUFFER, m_Binding, m_ID);
		glBindBuffer(GL_UNIFORM_BUFFER, 0);
	}

	void OpenGLUniformBuffer::BindToShader(uint32_t programID, const std::string& blockName)
	{
		GLuint index = glGetUniformBlockIndex(programID, blockName.c_str());
		if (index == GL_INVALID_INDEX)
		{
			std::cerr << "Uniform block '" << blockName << "' not found in shader." << std::endl;
			return;
		}

		glUniformBlockBinding(programID, index, m_Binding);
	}

	GLuint OpenGLUniformBuffer::GetID() const
	{
		return m_ID;
	}

	OpenGLVertexBuffer::OpenGLVertexBuffer()
	{
	}

	OpenGLVertexBuffer::OpenGLVertexBuffer(uint32_t size)
		: m_Size(size)
	{
		glCreateBuffers(1, &m_RendererID);
		glBindBuffer(GL_ARRAY_BUFFER, m_RendererID);
		glBufferData(GL_ARRAY_BUFFER, size, nullptr, GL_DYNAMIC_DRAW);
	}

	OpenGLVertexBuffer::OpenGLVertexBuffer(const std::vector<float>& vertices)
		: m_Size(vertices.size())
	{
		glCreateBuffers(1, &m_RendererID);
		glBindBuffer(GL_ARRAY_BUFFER, m_RendererID);
		glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(float), &vertices[0], GL_STATIC_DRAW);
	}

	OpenGLVertexBuffer::~OpenGLVertexBuffer()
	{
		glDeleteBuffers(1, &m_RendererID);
	}

	void OpenGLVertexBuffer::Bind() const
	{
		glBindBuffer(GL_ARRAY_BUFFER, m_RendererID);
	}

	void OpenGLVertexBuffer::Unbind() const
	{
		glBindBuffer(GL_ARRAY_BUFFER, 0);
	}

	void OpenGLVertexBuffer::UploadVertices(const std::vector<float> vertices)
	{
		glBindBuffer(GL_ARRAY_BUFFER, m_RendererID);
		glBufferSubData(GL_ARRAY_BUFFER, 0, vertices.size() * sizeof(float), vertices.data());
	}

	OpenGLIndexBuffer::OpenGLIndexBuffer()
	{
	}

	OpenGLIndexBuffer::OpenGLIndexBuffer(const std::vector<uint32_t> indices)
		: m_Count(indices.size())
	{
		glCreateBuffers(1, &m_RendererID);

		glBindBuffer(GL_ARRAY_BUFFER, m_RendererID);
		glBufferData(GL_ARRAY_BUFFER, indices.size() * sizeof(uint32_t), indices.data(), GL_STATIC_DRAW);
	}

	OpenGLIndexBuffer::~OpenGLIndexBuffer()
	{
		glDeleteBuffers(1, &m_RendererID);
	}

	void OpenGLIndexBuffer::Bind() const
	{
		glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, m_RendererID);
	}

	void OpenGLIndexBuffer::Unbind() const
	{
		glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
	}

	void OpenGLIndexBuffer::UploadIndices(const std::vector<uint32_t> indices)
	{
		glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, m_RendererID);
		glBufferSubData(GL_ELEMENT_ARRAY_BUFFER, 0, indices.size() * sizeof(uint32_t), indices.data());
	}
}