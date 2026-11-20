#pragma once

/// \file
/// \brief Shaders: the programs that the graphics card runs to draw, and the files they are read from.
/// \author Kurogenshi
/// \date 2026-11-18

#include <glm/mat3x3.hpp>
#include <glm/mat4x4.hpp>
#include <glm/vec2.hpp>
#include <glm/vec3.hpp>
#include <glm/vec4.hpp>

#include <cstdint>
#include <filesystem>
#include <optional>
#include <string>
#include <string_view>
#include <vector>

namespace Quasar
{
    /// @brief Stage of a shader, which runs at one step of the drawing.
    enum class ShaderStage : std::uint8_t
    {
        Vertex,  ///< Runs once per vertex and places it on the screen.
        Fragment ///< Runs once per pixel covered by a triangle and gives its color.
    };

    /// @brief Returns the name of a stage as written after `#type` in a shader file, for example "vertex".
    const char* GetShaderStageName(ShaderStage stage);

    /// @brief The GLSL code of one stage of a shader.
    struct ShaderSource
    {
        ShaderStage Stage = ShaderStage::Vertex; ///< The stage that the code is for.
        std::string Code;                        ///< The GLSL code, which starts with its `#version` line.
    };

    /// @brief Settings of a shader: its name and the code of its stages.
    ///
    /// The code is GLSL 4.50 core. The inputs of the vertex stage, the outputs of the fragment stage and the textures
    /// give their place with `layout(location = N)` and `layout(binding = N)`: nothing is looked up by name while
    /// drawing, and the same code can later be compiled to SPIR-V for Vulkan.
    struct ShaderSpecification
    {
        std::string Name;                  ///< Name of the shader, used in the logs.
        std::vector<ShaderSource> Sources; ///< The code of each stage. A vertex stage is required, and each stage appears once.

        /// @brief Reads a shader from text, where each stage starts with a line `#type vertex` or `#type fragment`.
        /// @param name The name of the shader.
        /// @param text The text, in which only empty lines can come before the first `#type`. Its `#include` lines are
        /// kept as they are: only FromFile() replaces them.
        /// @return The specification, or nothing if the text is not split into known stages. The reason is logged as an
        /// engine error.
        static std::optional<ShaderSpecification> Parse(std::string name, std::string_view text);

        /// @brief Reads a shader file, whose text is split as described in Parse(). The shader is named after the file,
        /// without its extension.
        ///
        /// A line `#include "File.glsl"` in a stage is replaced by the text of that file, found from the folder of the
        /// file that includes it. Included files can include other files. A file is inserted only once in each stage,
        /// so several files can include the same one, and the next includes of it are removed. `#line` directives are
        /// added around the inserted text, so the line numbers in the GLSL errors stay the ones of each file.
        /// @return The specification, or nothing if the file cannot be read or split, or if an include names no file,
        /// a file that cannot be read or a file that is already being included. The reason is logged as an engine
        /// error.
        static std::optional<ShaderSpecification> FromFile(const std::filesystem::path& file);
    };

    /// @brief A shader program, made of the compiled stages of a ShaderSpecification.
    ///
    /// A shader is created with GraphicsContext::CreateShader(), and Bind() makes the next draws use it. Its uniforms are
    /// set by name with the Set functions, which can be called whether the shader is bound or not. The names and types
    /// of the uniforms are read once when the shader is created, so setting one never asks the graphics driver. The
    /// name of a uniform array sets its first element.
    ///
    /// A Set function given a name that the shader does not use, or a value of another type than the uniform, does
    /// nothing and logs an engine warning, once per name. The GLSL compiler removes the uniforms that do not change the
    /// result, so a uniform declared but not used is unknown too.
    class Shader
    {
    public:
        virtual ~Shader() = default;

        Shader(const Shader&) = delete;
        Shader& operator=(const Shader&) = delete;
        Shader(Shader&&) = delete;
        Shader& operator=(Shader&&) = delete;

        /// @brief Returns true if a shader can be compiled from the specification, whatever the graphics API.
        ///
        /// It must have a vertex stage, and each stage only once. The reason a specification is refused is logged as an
        /// engine error.
        static bool IsValid(const ShaderSpecification& specification);

        /// @brief Makes the next draws use the shader.
        virtual void Bind() = 0;

        /// @brief Sets a uniform of type `int` or `bool`, or the texture unit of a sampler.
        virtual void SetInt(std::string_view name, int value) = 0;

        /// @brief Sets a uniform of type `float`.
        virtual void SetFloat(std::string_view name, float value) = 0;

        /// @brief Sets a uniform of type `vec2`.
        virtual void SetVec2(std::string_view name, glm::vec2 value) = 0;

        /// @brief Sets a uniform of type `vec3`.
        virtual void SetVec3(std::string_view name, glm::vec3 value) = 0;

        /// @brief Sets a uniform of type `vec4`.
        virtual void SetVec4(std::string_view name, glm::vec4 value) = 0;

        /// @brief Sets a uniform of type `mat3`.
        virtual void SetMat3(std::string_view name, const glm::mat3& value) = 0;

        /// @brief Sets a uniform of type `mat4`.
        virtual void SetMat4(std::string_view name, const glm::mat4& value) = 0;

        /// @brief Returns the name of the shader.
        const std::string& GetName() const { return m_Name; }

    protected:
        /// @brief Keeps the name of the shader.
        explicit Shader(std::string name);

    private:
        std::string m_Name;
    };
}
