#include "qepch.h"
#include "OpenGLBuffer.h"

#include <glad/glad.h>

#include <QuasarEngine/Core/Logger.h>

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
			Q_WARNING("Uniform buffer size mismatch: expected " + std::to_string(m_Size) + " bytes but got " + std::to_string(size) + " bytes.");
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
			//Q_ERROR("Uniform block '" + blockName + "' not found in shader.");
			return;
		}

		glUniformBlockBinding(programID, index, m_Binding);
	}

	GLuint OpenGLUniformBuffer::GetID() const
	{
		return m_ID;
	}

	OpenGLVertexBuffer::OpenGLVertexBuffer() : m_Size(0), m_Layout(), m_RendererID(0)
	{

	}

	OpenGLVertexBuffer::OpenGLVertexBuffer(uint32_t size)
		: m_Size(size)
	{
		glCreateBuffers(1, &m_RendererID);
		glBindBuffer(GL_ARRAY_BUFFER, m_RendererID);
		glBufferData(GL_ARRAY_BUFFER, size, nullptr, GL_DYNAMIC_DRAW);
	}

	OpenGLVertexBuffer::OpenGLVertexBuffer(const void* data, uint32_t size)
		: m_Size(size)
	{
		glCreateBuffers(1, &m_RendererID);
		glBindBuffer(GL_ARRAY_BUFFER, m_RendererID);
		glBufferData(GL_ARRAY_BUFFER, size, data, GL_STATIC_DRAW);
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

	void OpenGLVertexBuffer::Upload(const void* data, uint32_t size)
	{
		if (size == 0 || data == nullptr) return;

		if (size > m_Size) {
			Reserve(size);
		}

		glBindBuffer(GL_ARRAY_BUFFER, m_RendererID);
		glBufferSubData(GL_ARRAY_BUFFER, 0, size, data);
	}

	void OpenGLVertexBuffer::Reserve(uint32_t size)
	{
		if (size <= m_Size) return;
		m_Size = size;
		glBindBuffer(GL_ARRAY_BUFFER, m_RendererID);
		glBufferData(GL_ARRAY_BUFFER, m_Size, nullptr, GL_DYNAMIC_DRAW);
	}

	OpenGLIndexBuffer::OpenGLIndexBuffer() : m_Size(0), m_RendererID(0)
	{
	}

	OpenGLIndexBuffer::OpenGLIndexBuffer(uint32_t size)
		: m_Size(size)
	{
		glCreateBuffers(1, &m_RendererID);
		glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, m_RendererID);
		glBufferData(GL_ELEMENT_ARRAY_BUFFER, size, nullptr, GL_DYNAMIC_DRAW);
	}

	OpenGLIndexBuffer::OpenGLIndexBuffer(const void* data, uint32_t size)
		: m_Size(size)
	{
		glCreateBuffers(1, &m_RendererID);
		glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, m_RendererID);
		glBufferData(GL_ELEMENT_ARRAY_BUFFER, size, data, GL_STATIC_DRAW);
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

	void OpenGLIndexBuffer::Upload(const void* data, uint32_t size)
	{
		if (size == 0 || data == nullptr) return;

		if (size > m_Size) {
			Reserve(size);
		}

		glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, m_RendererID);
		glBufferSubData(GL_ELEMENT_ARRAY_BUFFER, 0, size, data);
	}

	void OpenGLIndexBuffer::Reserve(uint32_t size)
	{
		if (size <= m_Size) return;
		m_Size = size;
		glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, m_RendererID);
		glBufferData(GL_ELEMENT_ARRAY_BUFFER, m_Size, nullptr, GL_DYNAMIC_DRAW);
	}
}