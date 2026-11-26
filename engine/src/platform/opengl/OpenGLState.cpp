#include <platform/opengl/OpenGLState.h>

#include <algorithm>
#include <cstddef>

namespace Quasar
{
    namespace
    {
        GLenum GetCapability(OpenGLState::Capability capability)
        {
            switch (capability)
            {
                case OpenGLState::Capability::DepthTest:
                    return GL_DEPTH_TEST;
                case OpenGLState::Capability::CullFace:
                    return GL_CULL_FACE;
                case OpenGLState::Capability::Blend:
                    return GL_BLEND;
                case OpenGLState::Capability::ScissorTest:
                    return GL_SCISSOR_TEST;
            }
            return GL_DEPTH_TEST;
        }

        void Forget(std::optional<GLuint>& binding, GLuint name)
        {
            if (binding == name)
            {
                binding.reset();
            }
        }
    }

    OpenGLState::OpenGLState(RenderStatistics& statistics) : m_Statistics(&statistics) {}

    void OpenGLState::UseProgram(GLuint program)
    {
        if (Change(m_Program, program))
        {
            ++m_Statistics->StateChanges;
            glUseProgram(program);
        }
    }

    void OpenGLState::BindVertexArray(GLuint vertexArray)
    {
        if (Change(m_VertexArray, vertexArray))
        {
            ++m_Statistics->StateChanges;
            glBindVertexArray(vertexArray);
        }
    }

    void OpenGLState::BindFramebuffer(GLuint framebuffer)
    {
        if (Change(m_Framebuffer, framebuffer))
        {
            ++m_Statistics->FramebufferBinds;
            glBindFramebuffer(GL_FRAMEBUFFER, framebuffer);
        }
    }

    void OpenGLState::SetViewport(GLsizei width, GLsizei height)
    {
        if (Change(m_Viewport, {width, height}))
        {
            ++m_Statistics->StateChanges;
            glViewport(0, 0, width, height);
        }
    }

    void OpenGLState::BindTexture(GLuint slot, GLuint texture)
    {
        if (Change(At(m_Textures, slot), texture))
        {
            ++m_Statistics->TextureBinds;
            glBindTextureUnit(slot, texture);
        }
    }

    void OpenGLState::BindUniformBuffer(GLuint binding, GLuint buffer)
    {
        if (Change(At(m_UniformBuffers, binding), {.Buffer = buffer}))
        {
            ++m_Statistics->UniformBufferBinds;
            glBindBufferBase(GL_UNIFORM_BUFFER, binding, buffer);
        }
    }

    void OpenGLState::BindUniformBufferRange(GLuint binding, GLuint buffer, GLintptr offset, GLsizeiptr size)
    {
        if (Change(At(m_UniformBuffers, binding), {.Buffer = buffer, .Offset = offset, .Size = size}))
        {
            ++m_Statistics->UniformBufferBinds;
            glBindBufferRange(GL_UNIFORM_BUFFER, binding, buffer, offset, size);
        }
    }

    void OpenGLState::SetEnabled(Capability capability, bool enabled)
    {
        if (Change(m_Capabilities.at(static_cast<std::size_t>(capability)), enabled))
        {
            ++m_Statistics->StateChanges;
            if (enabled)
            {
                glEnable(GetCapability(capability));
            }
            else
            {
                glDisable(GetCapability(capability));
            }
        }
    }

    void OpenGLState::SetDepthWrite(bool enabled)
    {
        if (Change(m_DepthWrite, enabled))
        {
            ++m_Statistics->StateChanges;
            glDepthMask(enabled ? GL_TRUE : GL_FALSE);
        }
    }

    void OpenGLState::SetDepthFunction(GLenum function)
    {
        if (Change(m_DepthFunction, function))
        {
            ++m_Statistics->StateChanges;
            glDepthFunc(function);
        }
    }

    void OpenGLState::SetCullFace(GLenum face)
    {
        if (Change(m_CullFace, face))
        {
            ++m_Statistics->StateChanges;
            glCullFace(face);
        }
    }

    void OpenGLState::SetFrontFace(GLenum face)
    {
        if (Change(m_FrontFace, face))
        {
            ++m_Statistics->StateChanges;
            glFrontFace(face);
        }
    }

    void OpenGLState::SetBlendFunction(GLenum sourceColor, GLenum destinationColor, GLenum sourceAlpha, GLenum destinationAlpha)
    {
        if (Change(m_BlendFunction, {sourceColor, destinationColor, sourceAlpha, destinationAlpha}))
        {
            ++m_Statistics->StateChanges;
            glBlendFuncSeparate(sourceColor, destinationColor, sourceAlpha, destinationAlpha);
        }
    }

    void OpenGLState::SetBlendEquation(GLenum equation)
    {
        if (Change(m_BlendEquation, equation))
        {
            ++m_Statistics->StateChanges;
            glBlendEquation(equation);
        }
    }

    bool OpenGLState::IsDepthWriteEnabled()
    {
        if (!m_DepthWrite)
        {
            GLboolean enabled = GL_TRUE;
            glGetBooleanv(GL_DEPTH_WRITEMASK, &enabled);
            m_DepthWrite = enabled == GL_TRUE;
        }
        return *m_DepthWrite;
    }

    bool OpenGLState::IsEnabled(Capability capability)
    {
        std::optional<bool>& enabled = m_Capabilities.at(static_cast<std::size_t>(capability));
        if (!enabled)
        {
            enabled = glIsEnabled(GetCapability(capability)) == GL_TRUE;
        }
        return *enabled;
    }

    void OpenGLState::ForgetProgram(GLuint program)
    {
        Forget(m_Program, program);
    }

    void OpenGLState::ForgetVertexArray(GLuint vertexArray)
    {
        Forget(m_VertexArray, vertexArray);
    }

    void OpenGLState::ForgetFramebuffer(GLuint framebuffer)
    {
        Forget(m_Framebuffer, framebuffer);
    }

    void OpenGLState::ForgetTexture(GLuint texture)
    {
        std::ranges::for_each(m_Textures, [texture](std::optional<GLuint>& binding) { Forget(binding, texture); });
    }

    void OpenGLState::ForgetBuffer(GLuint buffer)
    {
        std::ranges::for_each(m_UniformBuffers,
                              [buffer](std::optional<UniformBufferBinding>& binding)
                              {
                                  if (binding && binding->Buffer == buffer)
                                  {
                                      binding.reset();
                                  }
                              });
    }

    void OpenGLState::Invalidate()
    {
        m_Program.reset();
        m_VertexArray.reset();
        m_Framebuffer.reset();
        m_Viewport.reset();
        m_Textures.clear();
        m_UniformBuffers.clear();
        m_Capabilities = {};
        m_DepthWrite.reset();
        m_DepthFunction.reset();
        m_CullFace.reset();
        m_FrontFace.reset();
        m_BlendFunction.reset();
        m_BlendEquation.reset();
    }
}
