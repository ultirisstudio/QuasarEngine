#include <platform/opengl/OpenGLVertexArray.h>

#include <platform/opengl/OpenGLBuffer.h>
#include <platform/opengl/OpenGLDevice.h>

#include <glad/gl.h>

#include <cstddef>
#include <cstdint>

namespace Quasar
{
    namespace
    {
        struct AttributeFormat
        {
            GLint Components = 1;
            bool Integer = false;
        };

        AttributeFormat GetFormat(VertexAttributeType type)
        {
            switch (type)
            {
                case VertexAttributeType::Float:
                    return {.Components = 1, .Integer = false};
                case VertexAttributeType::Vec2:
                    return {.Components = 2, .Integer = false};
                case VertexAttributeType::Vec3:
                case VertexAttributeType::Mat3:
                    return {.Components = 3, .Integer = false};
                case VertexAttributeType::Vec4:
                case VertexAttributeType::Mat4:
                    return {.Components = 4, .Integer = false};
                case VertexAttributeType::Int:
                    return {.Components = 1, .Integer = true};
                case VertexAttributeType::IVec2:
                    return {.Components = 2, .Integer = true};
                case VertexAttributeType::IVec3:
                    return {.Components = 3, .Integer = true};
                case VertexAttributeType::IVec4:
                    return {.Components = 4, .Integer = true};
            }
            return {};
        }
    }

    OpenGLVertexArray::OpenGLVertexArray(std::uint32_t vertexArray, OpenGLDevice& device) : m_VertexArray(vertexArray), m_Device(&device) {}

    OpenGLVertexArray::~OpenGLVertexArray()
    {
        m_Device->GetState().ForgetVertexArray(m_VertexArray);
        glDeleteVertexArrays(1, &m_VertexArray);
    }

    Scope<OpenGLVertexArray> OpenGLVertexArray::Create(OpenGLDevice& device)
    {
        GLuint vertexArray = 0;
        glCreateVertexArrays(1, &vertexArray);
        return Scope<OpenGLVertexArray>(new OpenGLVertexArray(vertexArray, device));
    }

    void OpenGLVertexArray::Bind() const
    {
        m_Device->GetState().BindVertexArray(m_VertexArray);
    }

    void OpenGLVertexArray::AttachVertexBuffer(const VertexBuffer& buffer, std::uint32_t index)
    {
        const VertexLayout& layout = buffer.GetLayout();
        // The buffers of an OpenGL context are OpenGL buffers
        // NOLINTNEXTLINE(cppcoreguidelines-pro-type-static-cast-downcast)
        const GLuint id = static_cast<const OpenGLVertexBuffer&>(buffer).GetId();
        glVertexArrayVertexBuffer(m_VertexArray, index, id, 0, static_cast<GLsizei>(layout.GetStride()));

        for (std::size_t attribute = 0; attribute < layout.GetAttributes().size(); ++attribute)
        {
            const VertexAttributeType type = layout.GetAttributes()[attribute].Type;
            const AttributeFormat format = GetFormat(type);
            const std::uint32_t columns = GetVertexAttributeLocationCount(type);
            const std::uint32_t columnSize = GetVertexAttributeSize(type) / columns;
            for (std::uint32_t column = 0; column < columns; ++column)
            {
                const GLuint location = layout.GetAttributes()[attribute].Location + column;
                const GLuint offset = layout.GetOffset(attribute) + column * columnSize;
                if (format.Integer)
                {
                    glVertexArrayAttribIFormat(m_VertexArray, location, format.Components, GL_INT, offset);
                }
                else
                {
                    glVertexArrayAttribFormat(m_VertexArray, location, format.Components, GL_FLOAT, GL_FALSE, offset);
                }
                glVertexArrayAttribBinding(m_VertexArray, location, index);
                glEnableVertexArrayAttrib(m_VertexArray, location);
            }
        }
    }

    void OpenGLVertexArray::AttachIndexBuffer(const IndexBuffer& buffer)
    {
        // NOLINTNEXTLINE(cppcoreguidelines-pro-type-static-cast-downcast)
        glVertexArrayElementBuffer(m_VertexArray, static_cast<const OpenGLIndexBuffer&>(buffer).GetId());
    }
}
