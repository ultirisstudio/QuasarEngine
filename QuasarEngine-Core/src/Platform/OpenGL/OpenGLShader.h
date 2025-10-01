#pragma once

#include "OpenGLBuffer.h"
#include "OpenGLTexture2D.h"

#include <algorithm>
#include <cstdint>
#include <cstring>
#include <memory>
#include <string>
#include <unordered_map>
#include <vector>

#include <QuasarEngine/Shader/Shader.h>
#include <glad/glad.h>

namespace QuasarEngine
{
    struct GLPipelineState {
        bool   depthTest = true;
        bool   depthWrite = true;
        GLenum depthFunc = GL_LESS;

        bool   cullEnabled = true;
        GLenum cullFace = GL_BACK;

        GLenum polygonMode = GL_FILL;

        bool   blendEnabled = false;
        GLenum blendSrcRGB = GL_ONE;
        GLenum blendDstRGB = GL_ZERO;
        GLenum blendSrcA = GL_ONE;
        GLenum blendDstA = GL_ZERO;
    };

    class RendererGLState {
    public:
        static RendererGLState& I() { static RendererGLState s; return s; }

        void PushCurrent();

        void Apply(const GLPipelineState& s);

        void Pop();

        void ApplyBaseline();

    private:
        std::vector<GLPipelineState> m_stack;
        GLPipelineState m_cached{};
        bool m_hasCached = false;
    };

    class OpenGLShader : public Shader
    {
    public:
        OpenGLShader(const ShaderDescription& desc);
        ~OpenGLShader() override;

        void Use() override;
        void Unuse() override;
        void Reset() override;

        bool UpdateGlobalState() override;
        bool UpdateObject(Material* material) override;

        bool AcquireResources(Material*) override { return true; }
        void ReleaseResources(Material*) override {}

        void SetUniform(const std::string& name, void* data, size_t size) override;
        void SetTexture(const std::string& name, Texture* texture, SamplerType type) override;

    private:
        void   LinkProgram(const std::vector<uint32_t>& shaders);
        void   ExtractUniformLocations();
        std::string ReadFile(const std::string& path);
        uint32_t    CompileShader(const std::string& source, uint32_t type);

        static GLenum DepthFuncToGL(Shader::DepthFunc func);
        static GLenum SamplerTypeToGL(Shader::SamplerType type);

        static std::string StripArraySuffix(const char* name, GLsizei len);

    private:
        uint32_t m_ID = 0;
        std::unordered_map<std::string, int> m_UniformLocations;

        std::unordered_map<std::string, const ShaderUniformDesc*> m_GlobalUniformMap;
        std::unordered_map<std::string, const ShaderUniformDesc*> m_ObjectUniformMap;

        std::unique_ptr<OpenGLUniformBuffer> m_GlobalUBO;
        std::unique_ptr<OpenGLUniformBuffer> m_ObjectUBO;
        std::vector<uint8_t> m_GlobalUniformData;
        std::vector<uint8_t> m_ObjectUniformData;

        std::unordered_map<std::string, OpenGLTexture2D*> m_ObjectTextures;
        std::unordered_map<std::string, Shader::SamplerType> m_ObjectTextureTypes;

        struct BoundTex { GLuint handle = 0; GLenum target = GL_TEXTURE_2D; };
        std::unordered_map<int, BoundTex> m_BoundPerUnit;

        OpenGLTexture2D* m_DefaultBlueTexture = nullptr;

        ShaderDescription m_Description;
    };
}