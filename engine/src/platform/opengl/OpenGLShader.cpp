#include <platform/opengl/OpenGLShader.h>

#include <platform/opengl/OpenGLState.h>

#include <quasar/log/Log.h>

#include <glad/gl.h>
#include <glm/gtc/type_ptr.hpp>

#include <algorithm>
#include <array>
#include <cstdint>
#include <initializer_list>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

namespace Quasar
{
    namespace
    {
        constexpr std::array<GLenum, 18> SamplerTypes{
            GL_SAMPLER_1D,
            GL_SAMPLER_2D,
            GL_SAMPLER_3D,
            GL_SAMPLER_CUBE,
            GL_SAMPLER_1D_SHADOW,
            GL_SAMPLER_2D_SHADOW,
            GL_SAMPLER_CUBE_SHADOW,
            GL_SAMPLER_1D_ARRAY,
            GL_SAMPLER_2D_ARRAY,
            GL_SAMPLER_2D_ARRAY_SHADOW,
            GL_SAMPLER_CUBE_MAP_ARRAY,
            GL_SAMPLER_2D_MULTISAMPLE,
            GL_SAMPLER_BUFFER,
            GL_INT_SAMPLER_2D,
            GL_INT_SAMPLER_2D_ARRAY,
            GL_UNSIGNED_INT_SAMPLER_2D,
            GL_UNSIGNED_INT_SAMPLER_2D_ARRAY,
            GL_IMAGE_2D,
        };

        GLenum GetStageType(ShaderStage stage)
        {
            switch (stage)
            {
                case ShaderStage::Vertex:
                    return GL_VERTEX_SHADER;
                case ShaderStage::Fragment:
                    return GL_FRAGMENT_SHADER;
            }
            return GL_VERTEX_SHADER;
        }

        std::string TrimLog(std::string log)
        {
            while (!log.empty() && (log.back() == '\0' || log.back() == '\n' || log.back() == '\r' || log.back() == ' '))
            {
                log.pop_back();
            }
            return log;
        }

        std::string GetShaderLog(GLuint shader)
        {
            GLint length = 0;
            glGetShaderiv(shader, GL_INFO_LOG_LENGTH, &length);
            std::string log(static_cast<std::size_t>(std::max(length, 0)), '\0');
            glGetShaderInfoLog(shader, length, nullptr, log.data());
            return TrimLog(std::move(log));
        }

        std::string GetProgramLog(GLuint program)
        {
            GLint length = 0;
            glGetProgramiv(program, GL_INFO_LOG_LENGTH, &length);
            std::string log(static_cast<std::size_t>(std::max(length, 0)), '\0');
            glGetProgramInfoLog(program, length, nullptr, log.data());
            return TrimLog(std::move(log));
        }

        GLuint CompileStage(const std::string& shaderName, const ShaderSource& source)
        {
            const GLuint shader = glCreateShader(GetStageType(source.Stage));
            const GLchar* code = source.Code.c_str();
            const auto length = static_cast<GLint>(source.Code.size());
            glShaderSource(shader, 1, &code, &length);
            glCompileShader(shader);

            GLint compiled = GL_FALSE;
            glGetShaderiv(shader, GL_COMPILE_STATUS, &compiled);
            if (compiled == GL_FALSE)
            {
                QUASAR_ENGINE_ERROR("Cannot compile the {} stage of the shader '{}':\n{}", GetShaderStageName(source.Stage), shaderName, GetShaderLog(shader));
                glDeleteShader(shader);
                return 0;
            }
            return shader;
        }

        GLuint LinkProgram(const ShaderSpecification& specification)
        {
            std::vector<GLuint> stages;
            for (const ShaderSource& source : specification.Sources)
            {
                const GLuint stage = CompileStage(specification.Name, source);
                if (stage == 0)
                {
                    std::ranges::for_each(stages, glDeleteShader);
                    return 0;
                }
                stages.push_back(stage);
            }

            const GLuint program = glCreateProgram();
            for (const GLuint stage : stages)
            {
                glAttachShader(program, stage);
            }
            glLinkProgram(program);
            for (const GLuint stage : stages)
            {
                glDetachShader(program, stage);
                glDeleteShader(stage);
            }

            GLint linked = GL_FALSE;
            glGetProgramiv(program, GL_LINK_STATUS, &linked);
            if (linked == GL_FALSE)
            {
                QUASAR_ENGINE_ERROR("Cannot link the shader '{}':\n{}", specification.Name, GetProgramLog(program));
                glDeleteProgram(program);
                return 0;
            }
            return program;
        }
    }

    OpenGLShader::OpenGLShader(std::string name, std::uint32_t program, OpenGLState& state) : Shader(std::move(name)), m_State(&state), m_Program(program) {}

    OpenGLShader::~OpenGLShader()
    {
        m_State->ForgetProgram(m_Program);
        glDeleteProgram(m_Program);
    }

    Scope<OpenGLShader> OpenGLShader::Create(const ShaderSpecification& specification, OpenGLState& state)
    {
        if (!IsValid(specification))
        {
            return nullptr;
        }

        const GLuint program = LinkProgram(specification);
        if (program == 0)
        {
            return nullptr;
        }

        Scope<OpenGLShader> shader(new OpenGLShader(specification.Name, program, state));
        shader->ReadUniforms();
        return shader;
    }

    void OpenGLShader::Bind()
    {
        m_State->UseProgram(m_Program);
    }

    void OpenGLShader::SetInt(std::string_view name, int value)
    {
        if (const GLint location = FindIntegerLocation(name); location >= 0)
        {
            ++m_State->GetStatistics().UniformChanges;
            glProgramUniform1i(m_Program, location, value);
        }
    }

    void OpenGLShader::SetFloat(std::string_view name, float value)
    {
        if (const GLint location = FindLocation(name, {GL_FLOAT}, "float"); location >= 0)
        {
            ++m_State->GetStatistics().UniformChanges;
            glProgramUniform1f(m_Program, location, value);
        }
    }

    void OpenGLShader::SetVec2(std::string_view name, glm::vec2 value)
    {
        if (const GLint location = FindLocation(name, {GL_FLOAT_VEC2}, "vec2"); location >= 0)
        {
            ++m_State->GetStatistics().UniformChanges;
            glProgramUniform2fv(m_Program, location, 1, glm::value_ptr(value));
        }
    }

    void OpenGLShader::SetVec3(std::string_view name, glm::vec3 value)
    {
        if (const GLint location = FindLocation(name, {GL_FLOAT_VEC3}, "vec3"); location >= 0)
        {
            ++m_State->GetStatistics().UniformChanges;
            glProgramUniform3fv(m_Program, location, 1, glm::value_ptr(value));
        }
    }

    void OpenGLShader::SetVec4(std::string_view name, glm::vec4 value)
    {
        if (const GLint location = FindLocation(name, {GL_FLOAT_VEC4}, "vec4"); location >= 0)
        {
            ++m_State->GetStatistics().UniformChanges;
            glProgramUniform4fv(m_Program, location, 1, glm::value_ptr(value));
        }
    }

    void OpenGLShader::SetMat3(std::string_view name, const glm::mat3& value)
    {
        if (const GLint location = FindLocation(name, {GL_FLOAT_MAT3}, "mat3"); location >= 0)
        {
            ++m_State->GetStatistics().UniformChanges;
            glProgramUniformMatrix3fv(m_Program, location, 1, GL_FALSE, glm::value_ptr(value));
        }
    }

    void OpenGLShader::SetMat4(std::string_view name, const glm::mat4& value)
    {
        if (const GLint location = FindLocation(name, {GL_FLOAT_MAT4}, "mat4"); location >= 0)
        {
            ++m_State->GetStatistics().UniformChanges;
            glProgramUniformMatrix4fv(m_Program, location, 1, GL_FALSE, glm::value_ptr(value));
        }
    }

    void OpenGLShader::ReadUniforms()
    {
        GLint count = 0;
        GLint maxNameLength = 0;
        glGetProgramInterfaceiv(m_Program, GL_UNIFORM, GL_ACTIVE_RESOURCES, &count);
        glGetProgramInterfaceiv(m_Program, GL_UNIFORM, GL_MAX_NAME_LENGTH, &maxNameLength);

        constexpr std::array<GLenum, 2> properties{GL_LOCATION, GL_TYPE};
        std::string name(static_cast<std::size_t>(std::max(maxNameLength, 1)), '\0');
        for (GLint index = 0; index < count; ++index)
        {
            std::array<GLint, properties.size()> values{};
            glGetProgramResourceiv(m_Program, GL_UNIFORM, static_cast<GLuint>(index), static_cast<GLsizei>(properties.size()), properties.data(),
                                   static_cast<GLsizei>(values.size()), nullptr, values.data());
            // The members of uniform blocks have no location: they are set through their buffer
            if (values[0] < 0)
            {
                continue;
            }

            GLsizei length = 0;
            glGetProgramResourceName(m_Program, GL_UNIFORM, static_cast<GLuint>(index), maxNameLength, &length, name.data());
            std::string uniformName(name.data(), static_cast<std::size_t>(length));
            // OpenGL names an array after its first element
            if (uniformName.ends_with("[0]"))
            {
                uniformName.resize(uniformName.size() - 3);
            }
            m_Uniforms.emplace(std::move(uniformName), Uniform{.Location = values[0], .Type = static_cast<std::uint32_t>(values[1])});
        }
    }

    GLint OpenGLShader::FindLocation(std::string_view name, std::initializer_list<std::uint32_t> types, const char* typeName)
    {
        const auto uniform = m_Uniforms.find(name);
        if (uniform != m_Uniforms.end() && std::ranges::find(types, uniform->second.Type) != types.end())
        {
            return uniform->second.Location;
        }

        if (m_ReportedNames.insert(std::string(name)).second)
        {
            if (uniform == m_Uniforms.end())
            {
                QUASAR_ENGINE_WARN("The shader '{}' has no uniform '{}', or does not use it", GetName(), name);
            }
            else
            {
                QUASAR_ENGINE_WARN("The uniform '{}' of the shader '{}' is not a {}", name, GetName(), typeName);
            }
        }
        return -1;
    }

    GLint OpenGLShader::FindIntegerLocation(std::string_view name)
    {
        const auto uniform = m_Uniforms.find(name);
        if (uniform != m_Uniforms.end() && std::ranges::find(SamplerTypes, uniform->second.Type) != SamplerTypes.end())
        {
            return uniform->second.Location;
        }
        return FindLocation(name, {GL_INT, GL_BOOL}, "int, bool or sampler");
    }
}
