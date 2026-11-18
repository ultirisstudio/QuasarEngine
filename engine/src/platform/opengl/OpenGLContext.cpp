#include <quasar/platform/opengl/OpenGLContext.h>

#include <platform/opengl/OpenGLBuffer.h>
#include <platform/opengl/OpenGLFramebuffer.h>
#include <platform/opengl/OpenGLShader.h>
#include <platform/opengl/OpenGLTexture.h>
#include <platform/opengl/OpenGLVertexArray.h>

#include <glad/gl.h>

#include <SDL3/SDL_error.h>
#include <SDL3/SDL_video.h>

#include <quasar/log/Log.h>

#include <cstddef>
#include <cstdint>
#include <span>

namespace Quasar
{
    namespace
    {
        constexpr int RequiredMajorVersion = 4;
        constexpr int RequiredMinorVersion = 5;

        const char* GetString(GLenum name)
        {
            const GLubyte* text = glGetString(name);
            // OpenGL returns its strings as unsigned characters, which hold the same bytes as characters
            // NOLINTNEXTLINE(cppcoreguidelines-pro-type-reinterpret-cast)
            return text != nullptr ? reinterpret_cast<const char*>(text) : "unknown";
        }

        const OpenGLVertexArray& GetOpenGLVertexArray(const VertexArray& vertexArray)
        {
            // The vertex arrays of an OpenGL context are OpenGL vertex arrays
            // NOLINTNEXTLINE(cppcoreguidelines-pro-type-static-cast-downcast)
            return static_cast<const OpenGLVertexArray&>(vertexArray);
        }
    }

    OpenGLContext::OpenGLContext(SDL_Window* window, SDL_GLContextState* context) : m_Window(window), m_Context(context) {}

    OpenGLContext::~OpenGLContext()
    {
        SDL_GL_DestroyContext(m_Context);
    }

    void OpenGLContext::SetAttributes()
    {
        SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, RequiredMajorVersion);
        SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, RequiredMinorVersion);
        SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE);
        SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);
        SDL_GL_SetAttribute(SDL_GL_DEPTH_SIZE, 24);
        SDL_GL_SetAttribute(SDL_GL_STENCIL_SIZE, 8);
    }

    Scope<OpenGLContext> OpenGLContext::Create(SDL_Window* window)
    {
        SDL_GLContext context = SDL_GL_CreateContext(window);
        if (context == nullptr)
        {
            QUASAR_ENGINE_ERROR("Cannot create an OpenGL {}.{} core context: {}. Update the graphics driver", RequiredMajorVersion, RequiredMinorVersion,
                                SDL_GetError());
            return nullptr;
        }

        Scope<OpenGLContext> created(new OpenGLContext(window, context));

        const int version = gladLoadGL(SDL_GL_GetProcAddress);
        if (version == 0)
        {
            QUASAR_ENGINE_ERROR("Cannot load the OpenGL functions. OpenGL {}.{} is required: update the graphics driver", RequiredMajorVersion,
                                RequiredMinorVersion);
            return nullptr;
        }

        if (GLAD_GL_VERSION_4_5 == 0)
        {
            QUASAR_ENGINE_ERROR("OpenGL {}.{} is required, but the graphics driver only provides OpenGL {}.{}: update the graphics driver",
                                RequiredMajorVersion, RequiredMinorVersion, GLAD_VERSION_MAJOR(version), GLAD_VERSION_MINOR(version));
            return nullptr;
        }

        glClipControl(GL_LOWER_LEFT, GL_ZERO_TO_ONE);

        QUASAR_ENGINE_INFO("OpenGL vendor: {}", GetString(GL_VENDOR));
        QUASAR_ENGINE_INFO("OpenGL renderer: {}", GetString(GL_RENDERER));
        QUASAR_ENGINE_INFO("OpenGL version: {}", GetString(GL_VERSION));

        return created;
    }

    void OpenGLContext::SwapBuffers()
    {
        SDL_GL_SwapWindow(m_Window);
    }

    void OpenGLContext::SetVSync(bool enabled)
    {
        SDL_GL_MakeCurrent(m_Window, m_Context);
        SDL_GL_SetSwapInterval(enabled ? 1 : 0);
    }

    Scope<Framebuffer> OpenGLContext::CreateFramebuffer(const FramebufferSpecification& specification)
    {
        SDL_GL_MakeCurrent(m_Window, m_Context);
        return OpenGLFramebuffer::Create(specification);
    }

    Scope<Shader> OpenGLContext::CreateShader(const ShaderSpecification& specification)
    {
        SDL_GL_MakeCurrent(m_Window, m_Context);
        return OpenGLShader::Create(specification);
    }

    Scope<VertexBuffer> OpenGLContext::CreateVertexBuffer(std::span<const std::byte> vertices, const VertexLayout& layout)
    {
        SDL_GL_MakeCurrent(m_Window, m_Context);
        return OpenGLVertexBuffer::Create(vertices, layout);
    }

    Scope<IndexBuffer> OpenGLContext::CreateIndexBuffer(std::span<const std::uint32_t> indices)
    {
        SDL_GL_MakeCurrent(m_Window, m_Context);
        return OpenGLIndexBuffer::Create(indices);
    }

    Scope<UniformBuffer> OpenGLContext::CreateUniformBuffer(std::size_t size, std::uint32_t binding)
    {
        SDL_GL_MakeCurrent(m_Window, m_Context);
        return OpenGLUniformBuffer::Create(size, binding);
    }

    Scope<Texture2D> OpenGLContext::CreateTexture2D(const TextureSpecification& specification, std::span<const std::byte> pixels)
    {
        SDL_GL_MakeCurrent(m_Window, m_Context);
        return OpenGLTexture2D::Create(specification, pixels);
    }

    Scope<VertexArray> OpenGLContext::CreateVertexArray()
    {
        SDL_GL_MakeCurrent(m_Window, m_Context);
        return OpenGLVertexArray::Create();
    }

    void OpenGLContext::Draw(const VertexArray& vertexArray, std::uint32_t vertexCount)
    {
        for (std::size_t index = 0; index < vertexArray.GetVertexBufferCount(); ++index)
        {
            const std::uint32_t available = vertexArray.GetVertexBuffer(index).GetVertexCount();
            if (vertexCount > available)
            {
                QUASAR_ENGINE_ERROR("Cannot draw {} vertices: the vertex buffer {} of the vertex array only holds {}", vertexCount, index, available);
                return;
            }
        }

        SDL_GL_MakeCurrent(m_Window, m_Context);
        GetOpenGLVertexArray(vertexArray).Bind();
        glDrawArrays(GL_TRIANGLES, 0, static_cast<GLsizei>(vertexCount));
    }

    void OpenGLContext::DrawIndexed(const VertexArray& vertexArray)
    {
        const IndexBuffer* indexBuffer = vertexArray.GetIndexBuffer();
        if (indexBuffer == nullptr)
        {
            QUASAR_ENGINE_ERROR("Cannot draw the indices of a vertex array that has no index buffer");
            return;
        }

        SDL_GL_MakeCurrent(m_Window, m_Context);
        GetOpenGLVertexArray(vertexArray).Bind();
        glDrawElements(GL_TRIANGLES, static_cast<GLsizei>(indexBuffer->GetCount()), GL_UNSIGNED_INT, nullptr);
    }
}
