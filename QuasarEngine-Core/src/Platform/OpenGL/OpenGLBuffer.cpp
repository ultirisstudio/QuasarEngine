#include "qepch.h"
#include "OpenGLBuffer.h"

#include <glad/glad.h>

#include <QuasarEngine/Core/Logger.h>

namespace QuasarEngine
{
	OpenGLUniformBuffer::OpenGLUniformBuffer(size_t size, uint32_t binding)
		: m_Size(size), m_Binding(binding)
	{
		if (size > 0) {
			glCreateBuffers(1, &m_ID);
			glNamedBufferData(m_ID, size, nullptr, GL_DYNAMIC_DRAW);
			glBindBufferBase(GL_UNIFORM_BUFFER, binding, m_ID);
		}
	}

	OpenGLUniformBuffer::~OpenGLUniformBuffer()
	{
		glDeleteBuffers(1, &m_ID);
	}

	void OpenGLUniformBuffer::SetData(const void* data, size_t size)
	{
		if (m_ID == 0 || !data || size == 0) return;

		if (size > m_Size)
			throw std::runtime_error("Uniform buffer size exceeded: requested " +
				std::to_string(size) + " > " + std::to_string(m_Size));

		if (size != m_Size) {
			Q_WARNING("UBO size mismatch: expected " + std::to_string(m_Size) +
				" got " + std::to_string(size));
		}

		glNamedBufferData(m_ID, m_Size, nullptr, GL_DYNAMIC_DRAW);
		glNamedBufferSubData(m_ID, 0, size, data);
	}

	void OpenGLUniformBuffer::BindToShader(uint32_t programID, const std::string& blockName)
	{
		if (m_ID == 0) return;

		GLuint index = glGetUniformBlockIndex(programID, blockName.c_str());
		if (index == GL_INVALID_INDEX) {
			Q_ERROR("Uniform block '" + blockName + "' not found in shader.");
			return;
		}
		glUniformBlockBinding(programID, index, m_Binding);
	}


	GLuint OpenGLUniformBuffer::GetID() const
	{
		return m_ID;
	}

	OpenGLVertexBuffer::OpenGLVertexBuffer() : m_Size(0), m_Layout(), m_ID(0)
	{

	}

	OpenGLVertexBuffer::OpenGLVertexBuffer(uint32_t size)
		: m_Size(size)
	{
		glCreateBuffers(1, &m_ID);
		glNamedBufferData(m_ID, size, nullptr, GL_DYNAMIC_DRAW);
	}

	OpenGLVertexBuffer::OpenGLVertexBuffer(const void* data, uint32_t size)
		: m_Size(size)
	{
		glCreateBuffers(1, &m_ID);
		glNamedBufferData(m_ID, size, data, GL_STATIC_DRAW);
	}

	OpenGLVertexBuffer::~OpenGLVertexBuffer()
	{
		glDeleteBuffers(1, &m_ID);
	}

	void OpenGLVertexBuffer::Bind() const
	{
		glBindBuffer(GL_ARRAY_BUFFER, m_ID);
	}

	void OpenGLVertexBuffer::Unbind() const
	{
		glBindBuffer(GL_ARRAY_BUFFER, 0);
	}

	void OpenGLVertexBuffer::EnablePersistentMapping(uint32_t size)
	{
		if (m_ID) glDeleteBuffers(1, &m_ID);
		glCreateBuffers(1, &m_ID);
		m_Size = size;
		const GLbitfield flags = GL_MAP_WRITE_BIT | GL_MAP_PERSISTENT_BIT | GL_MAP_COHERENT_BIT;
		glNamedBufferStorage(m_ID, m_Size, nullptr, flags);
		m_Mapped = glMapNamedBufferRange(m_ID, 0, m_Size, flags);
		m_IsPersistent = (m_Mapped != nullptr);
	}

	void OpenGLVertexBuffer::Upload(const void* data, uint32_t size)
	{
		if (!data || size == 0) return;

		if (size > m_Size) {
			Reserve(size);
		}

		if (m_IsPersistent && m_Mapped) {
			std::memcpy(m_Mapped, data, size);
		}
		else {
			glNamedBufferSubData(m_ID, 0, size, data);
		}
	}

	void OpenGLVertexBuffer::Upload(const void* data, uint32_t size, uint32_t offset)
	{
		if (!data || size == 0) return;
		const uint32_t end = offset + size;
		if (end > m_Size) Reserve(end);

		if (m_IsPersistent && m_Mapped) {
			std::memcpy(static_cast<std::byte*>(m_Mapped) + offset, data, size);
		}
		else {
			glNamedBufferSubData(m_ID, offset, size, data);
		}
	}

	void OpenGLVertexBuffer::Reserve(uint32_t size)
	{
		if (size <= m_Size) return;
		m_Size = size;

		if (m_IsPersistent) {
			EnablePersistentMapping(m_Size);
			return;
		}

		glNamedBufferData(m_ID, m_Size, nullptr, GL_DYNAMIC_DRAW);
	}

	OpenGLIndexBuffer::OpenGLIndexBuffer() : m_Size(0), m_ID(0)
	{
	}

	OpenGLIndexBuffer::OpenGLIndexBuffer(uint32_t size)
		: m_Size(size)
	{
		glCreateBuffers(1, &m_ID);
		glNamedBufferData(m_ID, size, nullptr, GL_DYNAMIC_DRAW);
	}

	OpenGLIndexBuffer::OpenGLIndexBuffer(const void* data, uint32_t size)
		: m_Size(size)
	{
		glCreateBuffers(1, &m_ID);
		glNamedBufferData(m_ID, size, data, GL_STATIC_DRAW);
	}

	OpenGLIndexBuffer::~OpenGLIndexBuffer()
	{
		glDeleteBuffers(1, &m_ID);
	}

	void OpenGLIndexBuffer::Bind() const
	{
		glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, m_ID);
	}

	void OpenGLIndexBuffer::Unbind() const
	{
		glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
	}

	void OpenGLIndexBuffer::EnablePersistentMapping(uint32_t size)
	{
		if (m_ID) glDeleteBuffers(1, &m_ID);
		glCreateBuffers(1, &m_ID);
		m_Size = size;
		const GLbitfield flags = GL_MAP_WRITE_BIT | GL_MAP_PERSISTENT_BIT | GL_MAP_COHERENT_BIT;
		glNamedBufferStorage(m_ID, m_Size, nullptr, flags);
		m_Mapped = glMapNamedBufferRange(m_ID, 0, m_Size, flags);
		m_IsPersistent = (m_Mapped != nullptr);
	}

	void OpenGLIndexBuffer::Upload(const void* data, uint32_t size)
	{
		if (!data || size == 0) return;

		if (size > m_Size) {
			Reserve(size);
		}

		if (m_IsPersistent && m_Mapped) {
			std::memcpy(m_Mapped, data, size);
		}
		else {
			glNamedBufferSubData(m_ID, 0, size, data);
		}
	}

	void OpenGLIndexBuffer::Upload(const void* data, uint32_t size, uint32_t offset)
	{
		if (!data || size == 0) return;
		const uint32_t end = offset + size;
		if (end > m_Size) Reserve(end);

		if (m_IsPersistent && m_Mapped) {
			std::memcpy(static_cast<std::byte*>(m_Mapped) + offset, data, size);
		}
		else {
			glNamedBufferSubData(m_ID, offset, size, data);
		}
	}

	void OpenGLIndexBuffer::Reserve(uint32_t size)
	{
		if (size <= m_Size) return;
		m_Size = size;

		if (m_IsPersistent) {
			EnablePersistentMapping(m_Size);
			return;
		}

		glNamedBufferData(m_ID, m_Size, nullptr, GL_DYNAMIC_DRAW);
	}
}