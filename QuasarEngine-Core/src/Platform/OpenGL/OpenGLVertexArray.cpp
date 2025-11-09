#include "qepch.h"
#include "OpenGLVertexArray.h"

#include <glad/glad.h>
#include "OpenGLBuffer.h"

namespace QuasarEngine
{
	static GLenum ShaderDataTypeToOpenGLBaseType(ShaderDataType type)
	{
		switch (type)
		{
		case ShaderDataType::Float:    return GL_FLOAT;
		case ShaderDataType::Vec2:   return GL_FLOAT;
		case ShaderDataType::Vec3:   return GL_FLOAT;
		case ShaderDataType::Vec4:   return GL_FLOAT;
		case ShaderDataType::Mat3:     return GL_FLOAT;
		case ShaderDataType::Mat4:     return GL_FLOAT;
		case ShaderDataType::Int:      return GL_INT;
        case ShaderDataType::Int4:    return GL_INT;
		case ShaderDataType::IVec2:     return GL_INT;
		case ShaderDataType::IVec3:     return GL_INT;
		case ShaderDataType::IVec4:     return GL_INT;
		case ShaderDataType::Bool:     return GL_UNSIGNED_BYTE;
		}

		return 0;
	}

	OpenGLVertexArray::OpenGLVertexArray()
		: m_RendererID(0), m_VertexBufferIndex(0)
	{
		glCreateVertexArrays(1, &m_RendererID);
	}

	OpenGLVertexArray::~OpenGLVertexArray()
	{
		glDeleteVertexArrays(1, &m_RendererID);
	}

	void OpenGLVertexArray::Bind() const
	{
		glBindVertexArray(m_RendererID);
	}

	void OpenGLVertexArray::Unbind() const
	{
		glBindVertexArray(0);
	}

    void OpenGLVertexArray::AddVertexBuffer(const std::shared_ptr<VertexBuffer>& vertexBuffer)
    {
        glBindVertexArray(m_RendererID);
        vertexBuffer->Bind();

        const auto& layout = vertexBuffer->GetLayout();
        for (const auto& element : layout)
        {
            const GLuint index = m_VertexBufferIndex;
            const GLsizei stride = layout.GetStride();
            const GLboolean norm = element.Normalized ? GL_TRUE : GL_FALSE;
            const GLvoid* ptr = (const void*)(uintptr_t)element.Offset;

            const GLenum base = ShaderDataTypeToOpenGLBaseType(element.Type);

            switch (element.Type)
            {
            case ShaderDataType::Float:
            case ShaderDataType::Vec2:
            case ShaderDataType::Vec3:
            case ShaderDataType::Vec4:
            {
                glEnableVertexAttribArray(index);
                glVertexAttribPointer(index, (GLint)element.GetComponentCount(),
                    base, norm, stride, ptr);
                m_VertexBufferIndex++;
                break;
            }

            case ShaderDataType::Int:
            case ShaderDataType::Int4:
            case ShaderDataType::IVec2:
            case ShaderDataType::IVec3:
            case ShaderDataType::IVec4:
            case ShaderDataType::Bool:
            {
                glEnableVertexAttribArray(index);
                glVertexAttribIPointer(index, (GLint)element.GetComponentCount(),
                    base, stride, ptr);
                m_VertexBufferIndex++;
                break;
            }

            case ShaderDataType::Mat3:
            case ShaderDataType::Mat4:
            {
                const GLint cols = (element.Type == ShaderDataType::Mat3) ? 3 : 4;
                for (GLint c = 0; c < cols; ++c)
                {
                    glEnableVertexAttribArray(m_VertexBufferIndex);
                    glVertexAttribPointer(
                        m_VertexBufferIndex,
                        cols,
                        GL_FLOAT,
                        norm,
                        stride,
                        (const void*)(element.Offset + sizeof(float) * cols * c)
                    );
                    m_VertexBufferIndex++;
                }
                break;
            }

            default: break;
            }
        }

        m_VertexBuffers.push_back(vertexBuffer);

        glBindVertexArray(0);
    }

    void OpenGLVertexArray::SetIndexBuffer(const std::shared_ptr<IndexBuffer>& indexBuffer)
    {
        m_IndexBuffer = indexBuffer;

        const auto* glIBO = static_cast<const OpenGLIndexBuffer*>(indexBuffer.get());
        glVertexArrayElementBuffer(m_RendererID, glIBO->GetID());
    }
}