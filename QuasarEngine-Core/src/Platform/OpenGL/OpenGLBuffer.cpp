#include "qepch.h"
#include "OpenGLBuffer.h"

#include <glad/glad.h>
#include <cstring>
#include <stdexcept>

#include <QuasarEngine/Core/Logger.h>

namespace QuasarEngine
{
	OpenGLUniformBuffer::OpenGLUniformBuffer(size_t size, uint32_t binding)
		: m_Size(size), m_Binding(binding)
	{
		m_ID = 0;
		if (m_Size > 0) {
			glCreateBuffers(1, &m_ID);
			glNamedBufferData(m_ID, m_Size, nullptr, GL_DYNAMIC_DRAW);
			glBindBufferBase(GL_UNIFORM_BUFFER, m_Binding, m_ID);
		}
	}

	OpenGLUniformBuffer::~OpenGLUniformBuffer()
	{
		if (m_ID) glDeleteBuffers(1, &m_ID);
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
		glBindBufferBase(GL_UNIFORM_BUFFER, m_Binding, m_ID);
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
		glBindBufferBase(GL_UNIFORM_BUFFER, m_Binding, m_ID);
	}

	GLuint OpenGLUniformBuffer::GetID() const { return m_ID; }

	OpenGLVertexBuffer::OpenGLVertexBuffer()
		: m_Size(0), m_Layout(), m_ID(0), m_IsPersistent(false), m_Mapped(nullptr)
	{
	}

	OpenGLVertexBuffer::OpenGLVertexBuffer(uint32_t size)
		: m_Size(size), m_Layout(), m_ID(0), m_IsPersistent(false), m_Mapped(nullptr)
	{
		glCreateBuffers(1, &m_ID);
		glNamedBufferData(m_ID, m_Size, nullptr, GL_DYNAMIC_DRAW);
	}

	OpenGLVertexBuffer::OpenGLVertexBuffer(const void* data, uint32_t size)
		: m_Size(size), m_Layout(), m_ID(0), m_IsPersistent(false), m_Mapped(nullptr)
	{
		glCreateBuffers(1, &m_ID);
		glNamedBufferData(m_ID, m_Size, data, GL_STATIC_DRAW);
	}

	OpenGLVertexBuffer::~OpenGLVertexBuffer()
	{
		if (m_IsPersistent && m_Mapped) {
			glUnmapNamedBuffer(m_ID);
			m_Mapped = nullptr;
		}
		if (m_ID) glDeleteBuffers(1, &m_ID);
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
		const GLbitfield storageFlags = GL_MAP_WRITE_BIT | GL_MAP_PERSISTENT_BIT | GL_MAP_COHERENT_BIT | GL_DYNAMIC_STORAGE_BIT;
		glNamedBufferStorage(m_ID, m_Size, nullptr, storageFlags);

		const GLbitfield mapFlags = GL_MAP_WRITE_BIT | GL_MAP_PERSISTENT_BIT | GL_MAP_COHERENT_BIT;
		m_Mapped = glMapNamedBufferRange(m_ID, 0, m_Size, mapFlags);
		m_IsPersistent = (m_Mapped != nullptr);
		if (!m_IsPersistent) {
			Q_ERROR("Persistent map failed for VBO. Falling back to subdata.");
		}
	}

	void OpenGLVertexBuffer::Upload(const void* data, uint32_t size)
	{
		if (!data || size == 0) return;

		if (size > m_Size) {
			Reserve(size);
		}

		if (m_IsPersistent && m_Mapped) {
			std::memcpy(m_Mapped, data, size);
			glMemoryBarrier(GL_CLIENT_MAPPED_BUFFER_BARRIER_BIT);
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
			std::memcpy(static_cast<char*>(m_Mapped) + offset, data, size);
			glMemoryBarrier(GL_CLIENT_MAPPED_BUFFER_BARRIER_BIT);
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

	OpenGLIndexBuffer::OpenGLIndexBuffer()
		: m_Size(0), m_ID(0), m_IsPersistent(false), m_Mapped(nullptr)
	{
	}

	OpenGLIndexBuffer::OpenGLIndexBuffer(uint32_t size)
		: m_Size(size), m_ID(0), m_IsPersistent(false), m_Mapped(nullptr)
	{
		glCreateBuffers(1, &m_ID);
		glNamedBufferData(m_ID, m_Size, nullptr, GL_DYNAMIC_DRAW);
	}

	OpenGLIndexBuffer::OpenGLIndexBuffer(const void* data, uint32_t size)
		: m_Size(size), m_ID(0), m_IsPersistent(false), m_Mapped(nullptr)
	{
		glCreateBuffers(1, &m_ID);
		glNamedBufferData(m_ID, m_Size, data, GL_STATIC_DRAW);
	}

	OpenGLIndexBuffer::~OpenGLIndexBuffer()
	{
		if (m_IsPersistent && m_Mapped) {
			glUnmapNamedBuffer(m_ID);
			m_Mapped = nullptr;
		}
		if (m_ID) glDeleteBuffers(1, &m_ID);
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
		const GLbitfield storageFlags = GL_MAP_WRITE_BIT | GL_MAP_PERSISTENT_BIT | GL_MAP_COHERENT_BIT | GL_DYNAMIC_STORAGE_BIT;
		glNamedBufferStorage(m_ID, m_Size, nullptr, storageFlags);

		const GLbitfield mapFlags = GL_MAP_WRITE_BIT | GL_MAP_PERSISTENT_BIT | GL_MAP_COHERENT_BIT;
		m_Mapped = glMapNamedBufferRange(m_ID, 0, m_Size, mapFlags);
		m_IsPersistent = (m_Mapped != nullptr);
		if (!m_IsPersistent) {
			Q_ERROR("Persistent map failed for IBO. Falling back to subdata.");
		}
	}

	void OpenGLIndexBuffer::Upload(const void* data, uint32_t size)
	{
		if (!data || size == 0) return;

		if (size > m_Size) {
			Reserve(size);
		}

		if (m_IsPersistent && m_Mapped) {
			std::memcpy(m_Mapped, data, size);
			glMemoryBarrier(GL_CLIENT_MAPPED_BUFFER_BARRIER_BIT);
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
			std::memcpy(static_cast<char*>(m_Mapped) + offset, data, size);
			glMemoryBarrier(GL_CLIENT_MAPPED_BUFFER_BARRIER_BIT);
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

	OpenGLShaderStorageBuffer::OpenGLShaderStorageBuffer(size_t size, uint32_t binding)
		: m_Size(0)
		, m_Binding(binding)
		, m_ID(0)
	{
		if (size > 0)
		{
			Reserve(size);
		}
	}

	OpenGLShaderStorageBuffer::~OpenGLShaderStorageBuffer()
	{
		if (m_ID)
		{
			glDeleteBuffers(1, &m_ID);
			m_ID = 0;
		}
	}

	void OpenGLShaderStorageBuffer::SetData(const void* data, size_t size)
	{
		if (!data || size == 0)
			return;

		if (m_ID == 0 || size > m_Size)
		{
			Reserve(size);
		}

		glNamedBufferSubData(m_ID, 0, size, data);

		glBindBufferBase(GL_SHADER_STORAGE_BUFFER, m_Binding, m_ID);
	}

	void OpenGLShaderStorageBuffer::BindToShader(uint32_t programID, const std::string& blockName)
	{
		if (m_ID == 0)
			return;

		GLuint index = glGetProgramResourceIndex(
			programID, GL_SHADER_STORAGE_BLOCK, blockName.c_str()
		);
		if (index == GL_INVALID_INDEX)
		{
			Q_ERROR("Shader storage block '" + blockName + "' not found in shader.");
			return;
		}

		glShaderStorageBlockBinding(programID, index, m_Binding);
		glBindBufferBase(GL_SHADER_STORAGE_BUFFER, m_Binding, m_ID);
	}

	void OpenGLShaderStorageBuffer::Reserve(size_t size)
	{
		if (size == 0)
			return;

		if (size <= m_Size && m_ID != 0)
			return;

		if (m_ID == 0)
		{
			glCreateBuffers(1, &m_ID);
		}

		m_Size = size;

		glNamedBufferData(m_ID, m_Size, nullptr, GL_DYNAMIC_DRAW);

		glBindBufferBase(GL_SHADER_STORAGE_BUFFER, m_Binding, m_ID);
	}
}
