#pragma once

#include <quasar/renderer/RenderStatistics.h>

#include <glad/gl.h>

#include <array>
#include <cstdint>
#include <optional>
#include <vector>

namespace Quasar
{
    class OpenGLState
    {
    public:
        enum class Capability : std::uint8_t
        {
            DepthTest,
            CullFace,
            Blend,
            ScissorTest,
        };

        explicit OpenGLState(RenderStatistics& statistics);

        RenderStatistics& GetStatistics() const { return *m_Statistics; }

        void UseProgram(GLuint program);
        void BindVertexArray(GLuint vertexArray);
        void BindFramebuffer(GLuint framebuffer);
        void SetViewport(GLsizei width, GLsizei height);
        void BindTexture(GLuint slot, GLuint texture);
        void BindUniformBuffer(GLuint binding, GLuint buffer);
        void SetEnabled(Capability capability, bool enabled);
        void SetDepthWrite(bool enabled);
        void SetDepthFunction(GLenum function);
        void SetCullFace(GLenum face);
        void SetFrontFace(GLenum face);
        void SetBlendFunction(GLenum sourceColor, GLenum destinationColor, GLenum sourceAlpha, GLenum destinationAlpha);
        void SetBlendEquation(GLenum equation);

        bool IsDepthWriteEnabled();
        bool IsEnabled(Capability capability);

        void ForgetProgram(GLuint program);
        void ForgetVertexArray(GLuint vertexArray);
        void ForgetFramebuffer(GLuint framebuffer);
        void ForgetTexture(GLuint texture);
        void ForgetBuffer(GLuint buffer);

        void Invalidate();

    private:
        template <typename T>
        bool Change(std::optional<T>& current, const T& value)
        {
            if (current == value)
            {
                return false;
            }
            current = value;
            return true;
        }

        static std::optional<GLuint>& At(std::vector<std::optional<GLuint>>& bindings, GLuint index);

        RenderStatistics* m_Statistics;
        std::optional<GLuint> m_Program;
        std::optional<GLuint> m_VertexArray;
        std::optional<GLuint> m_Framebuffer;
        std::optional<std::array<GLsizei, 2>> m_Viewport;
        std::vector<std::optional<GLuint>> m_Textures;
        std::vector<std::optional<GLuint>> m_UniformBuffers;
        std::array<std::optional<bool>, 4> m_Capabilities;
        std::optional<bool> m_DepthWrite;
        std::optional<GLenum> m_DepthFunction;
        std::optional<GLenum> m_CullFace;
        std::optional<GLenum> m_FrontFace;
        std::optional<std::array<GLenum, 4>> m_BlendFunction;
        std::optional<GLenum> m_BlendEquation;
    };
}
