#include <quasar/platform/opengl/OpenGLContext.h>

#include <platform/opengl/OpenGLBuffer.h>
#include <platform/opengl/OpenGLDevice.h>
#include <platform/opengl/OpenGLFramebuffer.h>
#include <platform/opengl/OpenGLPipeline.h>
#include <platform/opengl/OpenGLShader.h>
#include <platform/opengl/OpenGLTexture.h>
#include <platform/opengl/OpenGLTimestampQuery.h>
#include <platform/opengl/OpenGLVertexArray.h>

#include <glad/gl.h>
#include <glm/gtc/type_ptr.hpp>

#include <SDL3/SDL_error.h>
#include <SDL3/SDL_video.h>

#include <quasar/log/Log.h>

#include <algorithm>
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
        SDL_GL_MakeCurrent(m_Window, m_Context);
        ReleaseResources();
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
        created->m_Device = CreateScope<OpenGLDevice>(created->GetCurrentStatistics());

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
        return OpenGLFramebuffer::Create(specification, *m_Device);
    }

    Scope<Shader> OpenGLContext::CreateShader(const ShaderSpecification& specification)
    {
        SDL_GL_MakeCurrent(m_Window, m_Context);
        return OpenGLShader::Create(specification, *m_Device);
    }

    Scope<Pipeline> OpenGLContext::CreatePipeline(const PipelineSpecification& specification)
    {
        return OpenGLPipeline::Create(specification, *m_Device);
    }

    Scope<VertexBuffer> OpenGLContext::CreateVertexBuffer(std::span<const std::byte> vertices, const VertexLayout& layout)
    {
        SDL_GL_MakeCurrent(m_Window, m_Context);
        return OpenGLVertexBuffer::Create(vertices, layout, *m_Device);
    }

    Scope<IndexBuffer> OpenGLContext::CreateIndexBuffer(std::span<const std::uint32_t> indices)
    {
        SDL_GL_MakeCurrent(m_Window, m_Context);
        return OpenGLIndexBuffer::Create(indices, *m_Device);
    }

    Scope<UniformBuffer> OpenGLContext::CreateUniformBuffer(std::size_t size, std::uint32_t binding)
    {
        SDL_GL_MakeCurrent(m_Window, m_Context);
        return OpenGLUniformBuffer::Create(size, binding, *m_Device);
    }

    Scope<Texture2D> OpenGLContext::CreateTexture2D(const TextureSpecification& specification, std::span<const std::byte> pixels)
    {
        SDL_GL_MakeCurrent(m_Window, m_Context);
        return OpenGLTexture2D::Create(specification, pixels, *m_Device);
    }

    Scope<VertexArray> OpenGLContext::CreateVertexArray()
    {
        SDL_GL_MakeCurrent(m_Window, m_Context);
        return OpenGLVertexArray::Create(*m_Device);
    }

    Scope<TimestampQuery> OpenGLContext::CreateTimestampQuery()
    {
        SDL_GL_MakeCurrent(m_Window, m_Context);
        return OpenGLTimestampQuery::Create();
    }

    void OpenGLContext::BindWindow()
    {
        SDL_GL_MakeCurrent(m_Window, m_Context);
        int width = 0;
        int height = 0;
        SDL_GetWindowSizeInPixels(m_Window, &width, &height);
        m_Device->GetState().BindFramebuffer(0);
        m_Device->GetState().SetViewport(width, height);
    }

    std::uint32_t OpenGLContext::GetMaxSamples() const
    {
        const OpenGLLimits& limits = m_Device->GetLimits();
        return std::max(std::min(limits.Samples, limits.ColorTextureSamples), 1U);
    }

    float OpenGLContext::GetMaxAnisotropy() const
    {
        return m_Device->GetLimits().Anisotropy;
    }

    void OpenGLContext::InvalidateState()
    {
        m_Device->GetState().Invalidate();
    }

    glm::uvec2 OpenGLContext::GetWindowSize() const
    {
        int width = 0;
        int height = 0;
        SDL_GetWindowSizeInPixels(m_Window, &width, &height);
        return {static_cast<std::uint32_t>(std::max(width, 0)), static_cast<std::uint32_t>(std::max(height, 0))};
    }

    void OpenGLContext::ClearWindow(glm::vec4 color, float depth, std::uint8_t stencil)
    {
        SDL_GL_MakeCurrent(m_Window, m_Context);
        glClearNamedFramebufferfv(0, GL_COLOR, 0, glm::value_ptr(color));
        ClearOpenGLDepthStencil(m_Device->GetState(), 0, depth, stencil);
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

        RenderStatistics& statistics = GetCurrentStatistics();
        ++statistics.Draws;
        statistics.Triangles += vertexCount / 3;
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

        RenderStatistics& statistics = GetCurrentStatistics();
        ++statistics.Draws;
        statistics.Triangles += indexBuffer->GetCount() / 3;
        GetOpenGLVertexArray(vertexArray).Bind();
        glDrawElements(GL_TRIANGLES, static_cast<GLsizei>(indexBuffer->GetCount()), GL_UNSIGNED_INT, nullptr);
    }
}
