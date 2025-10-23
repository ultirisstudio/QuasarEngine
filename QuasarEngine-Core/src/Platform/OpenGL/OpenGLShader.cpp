#include "qepch.h"
#include "OpenGLShader.h"

#include <fstream>
#include <sstream>

#include <QuasarEngine/Core/Logger.h>
#include <glad/glad.h>

namespace QuasarEngine
{
    static void SetDepthAll(const GLPipelineState& s) {
        if (s.depthTest) glEnable(GL_DEPTH_TEST); else glDisable(GL_DEPTH_TEST);
        glDepthMask(s.depthWrite ? GL_TRUE : GL_FALSE);
        glDepthFunc(s.depthFunc);
    }

    static void SetCullAll(const GLPipelineState& s) {
        if (s.cullEnabled) { glEnable(GL_CULL_FACE); glCullFace(s.cullFace); }
        else glDisable(GL_CULL_FACE);
    }

    static void SetPolyAll(const GLPipelineState& s) {
        glPolygonMode(GL_FRONT_AND_BACK, s.polygonMode);
    }

    static void SetBlendAll(const GLPipelineState& s) {
        if (s.blendEnabled) {
            glEnable(GL_BLEND);
            glBlendFuncSeparate(s.blendSrcRGB, s.blendDstRGB, s.blendSrcA, s.blendDstA);
        }
        else {
            glDisable(GL_BLEND);
        }
    }

    static void SetPatchVertices(const GLPipelineState& s) {
        glPatchParameteri(GL_PATCH_VERTICES, s.patchVertices);
    }

    void RendererGLState::Apply(const GLPipelineState& s) {
        if (!m_hasCached) {
            SetDepthAll(s);
            SetCullAll(s);
            SetPolyAll(s);
            SetBlendAll(s);
            SetPatchVertices(s);

            m_cached = s;
            m_hasCached = true;

            return;
        }

        if (s.depthTest != m_cached.depthTest) (s.depthTest ? glEnable(GL_DEPTH_TEST) : glDisable(GL_DEPTH_TEST));
        if (s.depthWrite != m_cached.depthWrite) glDepthMask(s.depthWrite ? GL_TRUE : GL_FALSE);
        if (s.depthFunc != m_cached.depthFunc) glDepthFunc(s.depthFunc);

        if (s.cullEnabled != m_cached.cullEnabled) {
            if (s.cullEnabled) glEnable(GL_CULL_FACE); else glDisable(GL_CULL_FACE);
        }
        if (s.cullEnabled && s.cullFace != m_cached.cullFace) glCullFace(s.cullFace);

        if (s.polygonMode != m_cached.polygonMode)
            glPolygonMode(GL_FRONT_AND_BACK, s.polygonMode);

        if (s.blendEnabled != m_cached.blendEnabled) {
            if (s.blendEnabled) glEnable(GL_BLEND); else glDisable(GL_BLEND);
        }
        if (s.blendEnabled && (
            s.blendSrcRGB != m_cached.blendSrcRGB || s.blendDstRGB != m_cached.blendDstRGB ||
            s.blendSrcA != m_cached.blendSrcA || s.blendDstA != m_cached.blendDstA)) {
            glBlendFuncSeparate(s.blendSrcRGB, s.blendDstRGB, s.blendSrcA, s.blendDstA);
        }

        if (s.patchVertices != m_cached.patchVertices) {
            glPatchParameteri(GL_PATCH_VERTICES, s.patchVertices);
        }

        m_cached = s;
        m_hasCached = true;
    }

    void RendererGLState::ApplyBaseline() {
        GLPipelineState s{};
        s.depthTest = true; s.depthWrite = true; s.depthFunc = GL_LESS;
        s.cullEnabled = true; s.cullFace = GL_BACK;
        s.polygonMode = GL_FILL;
        s.blendEnabled = false;
        s.blendSrcRGB = GL_ONE; s.blendDstRGB = GL_ZERO; s.blendSrcA = GL_ONE; s.blendDstA = GL_ZERO;
        s.patchVertices = 1;
        Apply(s);
    }

    void RendererGLState::PushCurrent() {
        if (!m_hasCached) ApplyBaseline();
        m_stack.push_back(m_cached);
    }

    void RendererGLState::Pop() {
        if (m_stack.empty()) return;
        const GLPipelineState s = m_stack.back();
        m_stack.pop_back();
        Apply(s);
    }

    GLenum OpenGLShader::DepthFuncToGL(Shader::DepthFunc func)
    {
        switch (func)
        {
        case Shader::DepthFunc::Never:          return GL_NEVER;
        case Shader::DepthFunc::Less:           return GL_LESS;
        case Shader::DepthFunc::Equal:          return GL_EQUAL;
        case Shader::DepthFunc::LessOrEqual:    return GL_LEQUAL;
        case Shader::DepthFunc::Greater:        return GL_GREATER;
        case Shader::DepthFunc::NotEqual:       return GL_NOTEQUAL;
        case Shader::DepthFunc::GreaterOrEqual: return GL_GEQUAL;
        case Shader::DepthFunc::Always:         return GL_ALWAYS;
        default:                                return GL_LESS;
        }
    }

    GLenum OpenGLShader::SamplerTypeToGL(Shader::SamplerType type)
    {
        switch (type)
        {
        case Shader::SamplerType::Sampler2D:   return GL_TEXTURE_2D;
        case Shader::SamplerType::SamplerCube: return GL_TEXTURE_CUBE_MAP;
        default:                               return GL_TEXTURE_2D;
        }
    }

    std::string OpenGLShader::StripArraySuffix(const char* name, GLsizei len)
    {
        std::string s(name, len);
        if (s.size() >= 3 && s.compare(s.size() - 3, 3, "[0]") == 0) s.resize(s.size() - 3);
        return s;
    }

    std::string OpenGLShader::ReadFile(const std::string& path)
    {
        std::ifstream file(path);
        if (!file.is_open())
            throw std::runtime_error("Failed to open shader file: " + path);

        std::stringstream ss;
        ss << file.rdbuf();
        return ss.str();
    }

    uint32_t OpenGLShader::CompileShader(const std::string& source, uint32_t type)
    {
        const char* src = source.c_str();
        GLuint shader = glCreateShader(type);
        glShaderSource(shader, 1, &src, nullptr);
        glCompileShader(shader);

        GLint success = GL_FALSE;
        glGetShaderiv(shader, GL_COMPILE_STATUS, &success);
        if (!success)
        {
            char log[2048]; GLsizei len = 0;
            glGetShaderInfoLog(shader, sizeof(log), &len, log);
            std::string stage =
                (type == GL_VERTEX_SHADER ? "Vertex" :
                    type == GL_FRAGMENT_SHADER ? "Fragment" :
                    type == GL_GEOMETRY_SHADER ? "Geometry" :
                    type == GL_COMPUTE_SHADER ? "Compute" :
                    type == GL_TESS_CONTROL_SHADER ? "TessControl" :
                    type == GL_TESS_EVALUATION_SHADER ? "TessEval" : "Unknown");
            throw std::runtime_error("[" + stage + "] compile error:\n" + std::string(log, len));
        }
        return shader;
    }

    void OpenGLShader::LinkProgram(const std::vector<uint32_t>& shaders)
    {
        m_ID = glCreateProgram();
        for (uint32_t shader : shaders)
            glAttachShader(m_ID, shader);

        glLinkProgram(m_ID);

        GLint success = GL_FALSE;
        glGetProgramiv(m_ID, GL_LINK_STATUS, &success);
        if (!success)
        {
            char log[2048]; GLsizei len = 0;
            glGetProgramInfoLog(m_ID, sizeof(log), &len, log);
            throw std::runtime_error(std::string("Shader link error:\n") + std::string(log, len));
        }

        for (uint32_t shader : shaders)
        {
            glDetachShader(m_ID, shader);
            glDeleteShader(shader);
        }
    }

    void OpenGLShader::ExtractUniformLocations()
    {
        GLint count = 0;
        glGetProgramiv(m_ID, GL_ACTIVE_UNIFORMS, &count);

        char name[128];
        for (GLint i = 0; i < count; ++i)
        {
            GLsizei length = 0;
            GLint size = 0;
            GLenum type = 0;
            glGetActiveUniform(m_ID, i, sizeof(name), &length, &size, &type, name);
            std::string key = StripArraySuffix(name, length);
            GLint location = glGetUniformLocation(m_ID, key.c_str());
            m_UniformLocations[key] = location;
        }
    }

    OpenGLShader::OpenGLShader(const ShaderDescription& desc)
        : m_Description(desc)
    {
        size_t globalSize = 0;
        for (const auto& uniform : m_Description.globalUniforms)
            globalSize = std::max(globalSize, uniform.offset + uniform.size);
        m_GlobalUniformData.resize(globalSize);
        m_GlobalUBO = std::make_unique<OpenGLUniformBuffer>(globalSize, 0);

        size_t objectSize = 0;
        for (const auto& uniform : m_Description.objectUniforms)
            objectSize = std::max(objectSize, uniform.offset + uniform.size);
        m_ObjectUniformData.resize(objectSize);
        m_ObjectUBO = std::make_unique<OpenGLUniformBuffer>(objectSize, 1);

        for (const auto& uniform : m_Description.globalUniforms)
            m_GlobalUniformMap[uniform.name] = &uniform;
        for (const auto& uniform : m_Description.objectUniforms)
            m_ObjectUniformMap[uniform.name] = &uniform;

        std::vector<uint32_t> compiledShaders;
        compiledShaders.reserve(desc.modules.size());

        for (const auto& module : desc.modules)
        {
            std::string source;
            try {
                source = ReadFile(module.path);
            }
            catch (const std::exception& e) {
                Q_ERROR("Shader reading failed (" + module.path + "): " + std::string(e.what()));
                continue;
            }

            GLenum glStage = 0;
            switch (module.stage)
            {
            case ShaderStageType::Vertex:      glStage = GL_VERTEX_SHADER; break;
            case ShaderStageType::Fragment:    glStage = GL_FRAGMENT_SHADER; break;
            case ShaderStageType::Geometry:    glStage = GL_GEOMETRY_SHADER; break;
            case ShaderStageType::Compute:     glStage = GL_COMPUTE_SHADER; break;
            case ShaderStageType::TessControl: glStage = GL_TESS_CONTROL_SHADER; break;
            case ShaderStageType::TessEval:    glStage = GL_TESS_EVALUATION_SHADER; break;
            default: break;
            }
            if (!glStage) continue;

            try {
                uint32_t shader = CompileShader(source, glStage);
                compiledShaders.push_back(shader);
            }
            catch (const std::exception& e) {
                Q_ERROR("Compilation failed (" + module.path + "): " + std::string(e.what()));
            }
        }

        try {
            LinkProgram(compiledShaders);
        }
        catch (const std::exception& e) {
            Q_ERROR("Error linking shader : " + std::string(e.what()));
            throw;
        }

        ExtractUniformLocations();

        TextureSpecification spec;
        spec.width = 2;
        spec.height = 2;
        spec.format = TextureFormat::RGBA;
        spec.internal_format = TextureFormat::RGBA;
        spec.compressed = false;
        spec.alpha = true;
        spec.flip = false;
        spec.wrap_s = TextureWrap::REPEAT;
        spec.wrap_t = TextureWrap::REPEAT;
        spec.wrap_r = TextureWrap::REPEAT;
        spec.min_filter_param = TextureFilter::NEAREST;
        spec.mag_filter_param = TextureFilter::NEAREST;

        std::vector<unsigned char> bluePixels(4 * spec.width * spec.height, 0);
        for (int i = 0; i < spec.width * spec.height; ++i) {
            bluePixels[i * 4 + 0] = 0;   // R
            bluePixels[i * 4 + 1] = 0;   // G
            bluePixels[i * 4 + 2] = 255; // B
            bluePixels[i * 4 + 3] = 255; // A
        }
        m_DefaultBlueTexture = new OpenGLTexture2D(spec);
        m_DefaultBlueTexture->LoadFromData({ bluePixels.data(), bluePixels.size() });
    }

    OpenGLShader::~OpenGLShader()
    {
        if (m_ID) glDeleteProgram(m_ID);
        delete m_DefaultBlueTexture;
        m_DefaultBlueTexture = nullptr;
    }

    void OpenGLShader::Use()
    {
        static GLuint s_CurrentProgram = 0;
        if (s_CurrentProgram != m_ID) {
            glUseProgram(m_ID);
            s_CurrentProgram = m_ID;
        }

        m_BoundPerUnit.clear();

        RendererGLState::I().PushCurrent();

        GLPipelineState ps{};
        ps.depthTest = m_Description.depthTestEnable;
        ps.depthWrite = m_Description.depthWriteEnable;
        ps.depthFunc = DepthFuncToGL(m_Description.depthFunc);
        ps.patchVertices = m_Description.patchControlPoints;

        ps.cullEnabled = (m_Description.cullMode != CullMode::None);
        ps.cullFace = (m_Description.cullMode == CullMode::Back ? GL_BACK : GL_FRONT);

        ps.polygonMode = (m_Description.fillMode == FillMode::Wireframe ? GL_LINE : GL_FILL);

        if (m_Description.blendMode == BlendMode::None) {
            ps.blendEnabled = false;
        }
        else {
            ps.blendEnabled = true;
            switch (m_Description.blendMode) {
            case BlendMode::AlphaBlend:
                ps.blendSrcRGB = GL_SRC_ALPHA;            ps.blendDstRGB = GL_ONE_MINUS_SRC_ALPHA;
                ps.blendSrcA = GL_SRC_ALPHA;            ps.blendDstA = GL_ONE_MINUS_SRC_ALPHA;
                break;
            case BlendMode::Additive:
                ps.blendSrcRGB = GL_ONE;                  ps.blendDstRGB = GL_ONE;
                ps.blendSrcA = GL_ONE;                  ps.blendDstA = GL_ONE;
                break;
            case BlendMode::Multiply:
                ps.blendSrcRGB = GL_DST_COLOR;            ps.blendDstRGB = GL_ZERO;
                ps.blendSrcA = GL_DST_ALPHA;            ps.blendDstA = GL_ZERO;
                break;
            default:
                ps.blendEnabled = false; break;
            }
        }

        RendererGLState::I().Apply(ps);
    }

    void OpenGLShader::Unuse()
    {
        RendererGLState::I().Pop();
    }

    void OpenGLShader::Reset()
    {
        
    }

    bool OpenGLShader::UpdateGlobalState()
    {
        if (!m_GlobalUniformData.empty()) {
            m_GlobalUBO->SetData(m_GlobalUniformData.data(), m_GlobalUniformData.size());
        }
        m_GlobalUBO->BindToShader(m_ID, "global_uniform_object");
        return true;
    }

    bool OpenGLShader::UpdateObject(Material* /*material*/)
    {
        if (!m_ObjectUniformData.empty()) {
            m_ObjectUBO->SetData(m_ObjectUniformData.data(), m_ObjectUniformData.size());
        }
        m_ObjectUBO->BindToShader(m_ID, "local_uniform_object");

        for (const auto& samplerDesc : m_Description.samplers)
        {
            const std::string& sname = samplerDesc.name;

            SamplerType st = SamplerType::Sampler2D;
            if (auto itT = m_ObjectTextureTypes.find(sname); itT != m_ObjectTextureTypes.end())
                st = itT->second;

            Texture* texObj = m_DefaultBlueTexture;
            if (auto it = m_ObjectTextures.find(sname); it != m_ObjectTextures.end() && it->second)
                texObj = it->second;
            else if (st == SamplerType::SamplerCube)
                texObj = m_DefaultBlackCubemap;

            if (!texObj) { Q_ERROR("No texture bound for sampler " + sname); continue; }

            const GLuint handle = static_cast<GLuint>(texObj->GetHandle());
            if (handle == 0) { Q_ERROR("Invalid GL handle for sampler " + sname); continue; }

            const GLint unit = samplerDesc.binding;

            BoundTex& bound = m_BoundPerUnit[unit];
            if (bound.handle != handle) {
                glBindTextureUnit(unit, handle);
                bound.handle = handle;
            }

            if (auto itLoc = m_UniformLocations.find(sname); itLoc != m_UniformLocations.end() && itLoc->second >= 0)
                glUniform1i(itLoc->second, unit);
        }

        return true;
    }

    bool OpenGLShader::SetUniform(const std::string& name, void* data, size_t size)
    {
        if (auto gIt = m_GlobalUniformMap.find(name); gIt != m_GlobalUniformMap.end())
        {
            const auto* desc = gIt->second;
            const size_t copySize = std::min(size, desc->size);
            if (desc->offset + copySize <= m_GlobalUniformData.size())
                std::memcpy(m_GlobalUniformData.data() + desc->offset, data, copySize);
            return true;
        }

        if (auto oIt = m_ObjectUniformMap.find(name); oIt != m_ObjectUniformMap.end())
        {
            const auto* desc = oIt->second;
            const size_t copySize = std::min(size, desc->size);
            if (desc->offset + copySize <= m_ObjectUniformData.size())
                std::memcpy(m_ObjectUniformData.data() + desc->offset, data, copySize);
            return true;
        }

        Q_WARNING("Uniform not found : " + name);
        return false;
    }

    bool OpenGLShader::SetTexture(const std::string& name, Texture* texture, SamplerType type)
    {
        auto itSampler = std::find_if(
            m_Description.samplers.begin(), m_Description.samplers.end(),
            [&](const ShaderSamplerDesc& desc) { return desc.name == name; });

        if (itSampler == m_Description.samplers.end())
        {
            Q_ERROR("Sampler " + name + " not found in shader description!");
            return false;
        }

        m_ObjectTextures[name] = texture;
        m_ObjectTextureTypes[name] = type;

        return true;
    }
}
