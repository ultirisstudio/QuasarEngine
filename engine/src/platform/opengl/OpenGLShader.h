#pragma once

#include <quasar/core/Base.h>
#include <quasar/renderer/Shader.h>

#include <cstddef>
#include <cstdint>
#include <functional>
#include <initializer_list>
#include <string>
#include <string_view>
#include <unordered_map>
#include <unordered_set>

namespace Quasar
{
    class OpenGLState;

    class OpenGLShader final : public Shader
    {
    public:
        static Scope<OpenGLShader> Create(const ShaderSpecification& specification, OpenGLState& state);

        ~OpenGLShader() override;

        OpenGLShader(const OpenGLShader&) = delete;
        OpenGLShader& operator=(const OpenGLShader&) = delete;
        OpenGLShader(OpenGLShader&&) = delete;
        OpenGLShader& operator=(OpenGLShader&&) = delete;

        void Bind() override;

        void SetInt(std::string_view name, int value) override;
        void SetFloat(std::string_view name, float value) override;
        void SetVec2(std::string_view name, glm::vec2 value) override;
        void SetVec3(std::string_view name, glm::vec3 value) override;
        void SetVec4(std::string_view name, glm::vec4 value) override;
        void SetMat3(std::string_view name, const glm::mat3& value) override;
        void SetMat4(std::string_view name, const glm::mat4& value) override;

    private:
        struct Uniform
        {
            int Location = -1;
            std::uint32_t Type = 0;
        };

        struct NameHash
        {
            // The standard containers need this exact name to be searched with a std::string_view, without a copy
            using is_transparent = void; // NOLINT(readability-identifier-naming)
            std::size_t operator()(std::string_view name) const { return std::hash<std::string_view>{}(name); }
        };

        OpenGLShader(std::string name, std::uint32_t program, OpenGLState& state);

        void ReadUniforms();
        int FindLocation(std::string_view name, std::initializer_list<std::uint32_t> types, const char* typeName);
        int FindIntegerLocation(std::string_view name);

        OpenGLState* m_State;
        std::uint32_t m_Program = 0;
        std::unordered_map<std::string, Uniform, NameHash, std::equal_to<>> m_Uniforms;
        std::unordered_set<std::string, NameHash, std::equal_to<>> m_ReportedNames;
    };
}
