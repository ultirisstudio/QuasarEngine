#include "qepch.h"
#include "SkyboxHDR.h"

#include <cmath>
#include <cstdio>
#include <glad/glad.h>
#include <stb_image.h>
#include <glm/gtc/matrix_transform.hpp>

#include <QuasarEngine/Renderer/DrawMode.h>

namespace QuasarEngine
{
    static const char* VS_EXT() { return ".vert.glsl"; }
    static const char* FS_EXT() { return ".frag.glsl"; }

    static const char* GLErrorToStr(GLenum e) {
        switch (e) {
        case GL_NO_ERROR: return "GL_NO_ERROR";
        case GL_INVALID_ENUM: return "GL_INVALID_ENUM";
        case GL_INVALID_VALUE: return "GL_INVALID_VALUE";
        case GL_INVALID_OPERATION: return "GL_INVALID_OPERATION";
        case GL_STACK_OVERFLOW: return "GL_STACK_OVERFLOW";
        case GL_STACK_UNDERFLOW: return "GL_STACK_UNDERFLOW";
        case GL_OUT_OF_MEMORY: return "GL_OUT_OF_MEMORY";
        case GL_INVALID_FRAMEBUFFER_OPERATION: return "GL_INVALID_FRAMEBUFFER_OPERATION";
        default: return "GL_UNKNOWN_ERROR";
        }
    }

    static void LogGLErr(const char* where) {
        GLenum err;
        bool had = false;
        while ((err = glGetError()) != GL_NO_ERROR) {
            had = true;
            std::fprintf(stderr, "[SkyboxHDR][GL] %s : %s (0x%04X)\n", where, GLErrorToStr(err), err);
        }
        if (!had) std::fprintf(stderr, "[SkyboxHDR][GL] %s : OK\n", where);
    }

    static void LogFBOStatus(const char* where) {
        GLenum status = glCheckFramebufferStatus(GL_FRAMEBUFFER);
        std::fprintf(stderr, "[SkyboxHDR][FBO] %s : 0x%04X\n", where, status);
    }

    std::shared_ptr<SkyboxHDR> SkyboxHDR::Create(const Settings& s) {
        std::fprintf(stderr, "[SkyboxHDR] Create\n");
        return std::shared_ptr<SkyboxHDR>(new SkyboxHDR(s));
    }

    SkyboxHDR::SkyboxHDR(const Settings& s)
        : m_Settings(s)
    {
        std::fprintf(stderr, "[SkyboxHDR] Ctor\n");
        glEnable(GL_TEXTURE_CUBE_MAP_SEAMLESS);
        LogGLErr("glEnable(GL_TEXTURE_CUBE_MAP_SEAMLESS)");

        BuildCaptureViews();
        CreateShaders();
        CreateMeshes();
        CreateTargets();

        MaterialSpecification specification{};
        m_BackgroundMaterial = std::make_unique<Material>(specification);
        std::fprintf(stderr, "[SkyboxHDR] Material created %p\n", (void*)m_BackgroundMaterial.get());
    }

    SkyboxHDR::~SkyboxHDR() {
        std::fprintf(stderr, "[SkyboxHDR] Dtor\n");
        if (m_GL_RBO) { glDeleteRenderbuffers(1, &m_GL_RBO); LogGLErr("glDeleteRenderbuffers"); m_GL_RBO = 0; }
        if (m_GL_FBO) { glDeleteFramebuffers(1, &m_GL_FBO); LogGLErr("glDeleteFramebuffers"); m_GL_FBO = 0; }
        if (m_GL_HDRTex) { glDeleteTextures(1, &m_GL_HDRTex); LogGLErr("glDeleteTextures(HDRTex)");  m_GL_HDRTex = 0; }
    }

    bool SkyboxHDR::LoadFromHDR(const std::string& hdrPath) {
        std::fprintf(stderr, "[SkyboxHDR] LoadFromHDR: %s\n", hdrPath.c_str());
        return GenerateFromHDR(hdrPath);
    }

    void SkyboxHDR::Bind() { std::fprintf(stderr, "[SkyboxHDR] Bind\n"); }
    void SkyboxHDR::Unbind() { std::fprintf(stderr, "[SkyboxHDR] Unbind\n"); }

    void SkyboxHDR::Draw(const glm::mat4& view, const glm::mat4& projection) {
        if (!m_IsReady) { std::fprintf(stderr, "[SkyboxHDR] Draw skipped: not ready\n"); return; }
        if (!m_BackgroundShader) { std::fprintf(stderr, "[SkyboxHDR] Draw skipped: no background shader\n"); return; }
        if (!m_CubeMesh) { std::fprintf(stderr, "[SkyboxHDR] Draw skipped: no cube mesh\n"); return; }

        glm::mat4 v = glm::mat4(glm::mat3(view));
        glm::mat4 proj = projection;

        std::fprintf(stderr, "[SkyboxHDR] Draw begin\n");

        m_BackgroundShader->Use();
        std::fprintf(stderr, "[SkyboxHDR] Background Use\n");

        bool ok1 = m_BackgroundShader->SetUniform("view", &v, sizeof(glm::mat4));
        bool ok2 = m_BackgroundShader->SetUniform("projection", &proj, sizeof(glm::mat4));
        std::fprintf(stderr, "[SkyboxHDR] SetUniform view=%d projection=%d\n", (int)ok1, (int)ok2);

        bool gok = m_BackgroundShader->UpdateGlobalState();
        std::fprintf(stderr, "[SkyboxHDR] UpdateGlobalState=%d\n", (int)gok);

        m_BackgroundMaterial->SetTexture(TextureType::Albedo, m_EnvCubemap.get());
        bool stok = m_BackgroundShader->SetTexture("environmentMap",
            m_BackgroundMaterial->GetTexture(TextureType::Albedo),
            Shader::SamplerType::SamplerCube);
        std::fprintf(stderr, "[SkyboxHDR] SetTexture environmentMap=%d\n", (int)stok);

        bool ook = m_BackgroundShader->UpdateObject(m_BackgroundMaterial.get());
        std::fprintf(stderr, "[SkyboxHDR] UpdateObject=%d\n", (int)ook);

        GLboolean wasCull = glIsEnabled(GL_CULL_FACE);
        GLboolean wasDepth = glIsEnabled(GL_DEPTH_TEST);
        std::fprintf(stderr, "[SkyboxHDR] State before draw: cull=%d depth=%d\n", (int)wasCull, (int)wasDepth);

        if (wasCull) glDisable(GL_CULL_FACE);
        if (!wasDepth) glEnable(GL_DEPTH_TEST);
        glDepthFunc(GL_LEQUAL);
        LogGLErr("State setup for skybox");

        m_CubeMesh->draw();
        LogGLErr("Cube draw");

        glDepthFunc(GL_LESS);
        if (!wasDepth) glDisable(GL_DEPTH_TEST);
        if (wasCull)   glEnable(GL_CULL_FACE);
        LogGLErr("State restore after skybox");

        m_BackgroundShader->Unuse();
        std::fprintf(stderr, "[SkyboxHDR] Draw end\n");
    }

    void SkyboxHDR::BuildCaptureViews() {
        std::fprintf(stderr, "[SkyboxHDR] BuildCaptureViews\n");
        m_CaptureProj = glm::perspective(glm::radians(90.0f), 1.0f, 0.1f, 10.0f);
        m_CaptureViews[0] = glm::lookAt(glm::vec3(0.0f), glm::vec3(1, 0, 0), glm::vec3(0, -1, 0));
        m_CaptureViews[1] = glm::lookAt(glm::vec3(0.0f), glm::vec3(-1, 0, 0), glm::vec3(0, -1, 0));
        m_CaptureViews[2] = glm::lookAt(glm::vec3(0.0f), glm::vec3(0, 1, 0), glm::vec3(0, 0, 1));
        m_CaptureViews[3] = glm::lookAt(glm::vec3(0.0f), glm::vec3(0, -1, 0), glm::vec3(0, 0, -1));
        m_CaptureViews[4] = glm::lookAt(glm::vec3(0.0f), glm::vec3(0, 0, 1), glm::vec3(0, -1, 0));
        m_CaptureViews[5] = glm::lookAt(glm::vec3(0.0f), glm::vec3(0, 0, -1), glm::vec3(0, -1, 0));
    }

    void SkyboxHDR::CreateShaders() {
        const std::string base = "Assets/Shaders/gl/";

        {
            std::fprintf(stderr, "[SkyboxHDR] Create EquirectToCube shader\n");
            Shader::ShaderDescription d;

            d.modules = {
                {
                    Shader::ShaderStageType::Vertex,
                    base + "cubemap" + VS_EXT(),
                    { {0, Shader::ShaderIOType::Vec3, "inPosition", true, ""} }
                },
                { Shader::ShaderStageType::Fragment, base + "equirectangular_to_cubemap" + FS_EXT(), {} }
            };

            struct alignas(16) Globals { glm::mat4 projection; glm::mat4 view; };
            const auto flags = Shader::StageToBit(Shader::ShaderStageType::Vertex) |
                Shader::StageToBit(Shader::ShaderStageType::Fragment);

            d.globalUniforms = {
                { "projection", Shader::ShaderUniformType::Mat4, sizeof(glm::mat4), offsetof(Globals, projection), 0, 0, flags },
                { "view",       Shader::ShaderUniformType::Mat4, sizeof(glm::mat4), offsetof(Globals, view),       0, 0, flags },
            };

            d.samplers = {
                { "equirectangularMap", 1, 1, Shader::StageToBit(Shader::ShaderStageType::Fragment) }
            };

            d.depthTestEnable = true;
            d.depthWriteEnable = false;
            d.depthFunc = Shader::DepthFunc::LessOrEqual;
            d.cullMode = Shader::CullMode::Back;
            d.fillMode = Shader::FillMode::Solid;

            m_EquirectToCube = Shader::Create(d);
            std::fprintf(stderr, "[SkyboxHDR] EquirectToCube shader %p\n", (void*)m_EquirectToCube.get());
        }

        {
            std::fprintf(stderr, "[SkyboxHDR] Create Irradiance shader\n");
            Shader::ShaderDescription d;
            d.modules = {
                {
                    Shader::ShaderStageType::Vertex,
                    base + "cubemap" + VS_EXT(),
                    { {0, Shader::ShaderIOType::Vec3, "inPosition", true, ""} }
                },
                { Shader::ShaderStageType::Fragment, base + "irradiance_convolution" + FS_EXT(), {} }
            };

            struct alignas(16) Globals { glm::mat4 projection; glm::mat4 view; };
            const auto flags = Shader::StageToBit(Shader::ShaderStageType::Vertex) |
                Shader::StageToBit(Shader::ShaderStageType::Fragment);

            d.globalUniforms = {
                { "projection", Shader::ShaderUniformType::Mat4, sizeof(glm::mat4), offsetof(Globals, projection), 0, 0, flags },
                { "view",       Shader::ShaderUniformType::Mat4, sizeof(glm::mat4), offsetof(Globals, view),       0, 0, flags },
            };

            d.samplers = {
                { "environmentMap", 1, 2, Shader::StageToBit(Shader::ShaderStageType::Fragment) }
            };

            d.depthTestEnable = true;
            d.depthWriteEnable = false;
            d.depthFunc = Shader::DepthFunc::LessOrEqual;
            d.cullMode = Shader::CullMode::Back;
            d.fillMode = Shader::FillMode::Solid;

            m_Irradiance = Shader::Create(d);
            std::fprintf(stderr, "[SkyboxHDR] Irradiance shader %p\n", (void*)m_Irradiance.get());
        }

        {
            std::fprintf(stderr, "[SkyboxHDR] Create Prefilter shader\n");
            Shader::ShaderDescription d;
            d.modules = {
                {
                    Shader::ShaderStageType::Vertex,
                    base + "cubemap" + VS_EXT(),
                    { {0, Shader::ShaderIOType::Vec3, "inPosition", true, ""} }
                },
                { Shader::ShaderStageType::Fragment, base + "prefilter" + FS_EXT(), {} }
            };

            struct alignas(16) Globals { glm::mat4 projection; glm::mat4 view; };
            const auto gflags = Shader::StageToBit(Shader::ShaderStageType::Vertex) |
                Shader::StageToBit(Shader::ShaderStageType::Fragment);

            d.globalUniforms = {
                { "projection", Shader::ShaderUniformType::Mat4,  sizeof(glm::mat4), offsetof(Globals, projection), 0, 0, gflags },
                { "view",       Shader::ShaderUniformType::Mat4,  sizeof(glm::mat4), offsetof(Globals, view),       0, 0, gflags },
            };

            struct alignas(16) Obj { float roughness; };
            const auto oflags = Shader::StageToBit(Shader::ShaderStageType::Fragment) |
                Shader::StageToBit(Shader::ShaderStageType::Vertex);

            d.objectUniforms = {
                { "roughness", Shader::ShaderUniformType::Float, sizeof(float), offsetof(Obj, roughness), 1, 1, oflags }
            };

            d.samplers = {
                { "environmentMap", 1, 3, Shader::StageToBit(Shader::ShaderStageType::Fragment) }
            };

            d.depthTestEnable = true;
            d.depthWriteEnable = false;
            d.depthFunc = Shader::DepthFunc::LessOrEqual;
            d.cullMode = Shader::CullMode::Back;
            d.fillMode = Shader::FillMode::Solid;

            m_Prefilter = Shader::Create(d);
            std::fprintf(stderr, "[SkyboxHDR] Prefilter shader %p\n", (void*)m_Prefilter.get());
        }

        {
            std::fprintf(stderr, "[SkyboxHDR] Create BRDF shader\n");
            Shader::ShaderDescription d;
            d.modules = {
                {
                    Shader::ShaderStageType::Vertex,
                    base + "brdf" + VS_EXT(),
                    {
                        {0, Shader::ShaderIOType::Vec3, "inPosition", true, ""},
                        {1, Shader::ShaderIOType::Vec3, "inNormal",   true, ""},
                        {2, Shader::ShaderIOType::Vec2, "inTexCoord", true, ""},
                    }
                },
                { Shader::ShaderStageType::Fragment, base + "brdf" + FS_EXT(), {} }
            };

            d.depthTestEnable = true;
            d.depthWriteEnable = false;
            d.depthFunc = Shader::DepthFunc::LessOrEqual;
            d.cullMode = Shader::CullMode::Back;
            d.fillMode = Shader::FillMode::Solid;

            m_BRDF = Shader::Create(d);
            std::fprintf(stderr, "[SkyboxHDR] BRDF shader %p\n", (void*)m_BRDF.get());
        }

        {
            std::fprintf(stderr, "[SkyboxHDR] Create Background shader\n");
            Shader::ShaderDescription d;
            d.modules = {
                {
                    Shader::ShaderStageType::Vertex,
                    base + "background" + VS_EXT(),
                    { {0, Shader::ShaderIOType::Vec3, "inPosition", true, ""} }
                },
                { Shader::ShaderStageType::Fragment, base + "background" + FS_EXT(), {} }
            };

            struct alignas(16) Globals { glm::mat4 view; glm::mat4 projection; };
            const auto flags = Shader::StageToBit(Shader::ShaderStageType::Vertex) |
                Shader::StageToBit(Shader::ShaderStageType::Fragment);

            d.globalUniforms = {
                { "view",       Shader::ShaderUniformType::Mat4, sizeof(glm::mat4), offsetof(Globals, view),       0, 0, flags },
                { "projection", Shader::ShaderUniformType::Mat4, sizeof(glm::mat4), offsetof(Globals, projection), 0, 0, flags },
            };

            d.samplers = {
                { "environmentMap", 1, 1, Shader::StageToBit(Shader::ShaderStageType::Fragment) }
            };

            d.depthTestEnable = true;
            d.depthWriteEnable = false;
            d.depthFunc = Shader::DepthFunc::LessOrEqual;
            d.cullMode = Shader::CullMode::Back;
            d.fillMode = Shader::FillMode::Solid;

            m_BackgroundShader = Shader::Create(d);
            std::fprintf(stderr, "[SkyboxHDR] Background shader %p\n", (void*)m_BackgroundShader.get());
        }
    }

    void SkyboxHDR::CreateMeshes() {
        std::fprintf(stderr, "[SkyboxHDR] CreateMeshes\n");
        std::vector<float> cube;
        cube.reserve(36 * 3);
        auto tri = [&](glm::vec3 a, glm::vec3 b, glm::vec3 c) {
            cube.insert(cube.end(), { a.x,a.y,a.z, b.x,b.y,b.z, c.x,c.y,c.z });
            };

        tri({ -1,-1, 1 }, { 1,-1, 1 }, { 1, 1, 1 }); tri({ 1, 1, 1 }, { -1, 1, 1 }, { -1,-1, 1 });
        tri({ -1,-1,-1 }, { -1, 1,-1 }, { 1, 1,-1 }); tri({ 1, 1,-1 }, { 1,-1,-1 }, { -1,-1,-1 });
        tri({ -1, 1,-1 }, { -1, 1, 1 }, { 1, 1, 1 }); tri({ 1, 1, 1 }, { 1, 1,-1 }, { -1, 1,-1 });
        tri({ -1,-1,-1 }, { 1,-1,-1 }, { 1,-1, 1 }); tri({ 1,-1, 1 }, { -1,-1, 1 }, { -1,-1,-1 });
        tri({ 1,-1,-1 }, { 1, 1,-1 }, { 1, 1, 1 }); tri({ 1, 1, 1 }, { 1,-1, 1 }, { 1,-1,-1 });
        tri({ -1,-1,-1 }, { -1,-1, 1 }, { -1, 1, 1 }); tri({ -1, 1, 1 }, { -1, 1,-1 }, { -1,-1,-1 });

        BufferLayout cubeLayout = { { ShaderDataType::Vec3, "inPosition" } };
        m_CubeMesh = std::make_shared<Mesh>(
            cube, std::vector<unsigned int>{}, cubeLayout, DrawMode::TRIANGLES, std::nullopt
        );
        std::fprintf(stderr, "[SkyboxHDR] Cube mesh %p vtx=%zu\n", (void*)m_CubeMesh.get(), m_CubeMesh->GetVerticesCount());

        std::vector<float> quad; quad.reserve(4 * (3 + 3 + 2));
        auto pushV = [&](glm::vec3 p, glm::vec3 n, glm::vec2 uv) {
            quad.insert(quad.end(), { p.x,p.y,p.z, n.x,n.y,n.z, uv.x,uv.y });
            };
        pushV({ -1,  1, 0 }, { 0,0,1 }, { 0,1 });
        pushV({ -1, -1, 0 }, { 0,0,1 }, { 0,0 });
        pushV({ 1,  1, 0 }, { 0,0,1 }, { 1,1 });
        pushV({ 1, -1, 0 }, { 0,0,1 }, { 1,0 });

        BufferLayout quadLayout = {
            { ShaderDataType::Vec3, "inPosition" },
            { ShaderDataType::Vec3, "inNormal"   },
            { ShaderDataType::Vec2, "inTexCoord" },
        };
        m_QuadMesh = std::make_shared<Mesh>(
            quad, std::vector<unsigned int>{}, quadLayout, DrawMode::TRIANGLE_STRIP, std::nullopt
        );
        std::fprintf(stderr, "[SkyboxHDR] Quad mesh %p vtx=%zu\n", (void*)m_QuadMesh.get(), m_QuadMesh->GetVerticesCount());
    }

    void SkyboxHDR::CreateTargets() {
        std::fprintf(stderr, "[SkyboxHDR] CreateTargets\n");
        TextureSpecification s{};
        s.format = TextureFormat::RGBA;
        s.internal_format = TextureFormat::RGBA;
        s.wrap_r = s.wrap_s = s.wrap_t = TextureWrap::CLAMP_TO_EDGE;
        s.min_filter_param = TextureFilter::LINEAR_MIPMAP_LINEAR;
        s.mag_filter_param = TextureFilter::LINEAR;
        s.mipmap = true;
        s.compressed = false;
        s.alpha = true; s.gamma = false; s.channels = 4;

        {
            TextureSpecification se = s;
            se.width = m_Settings.envResolution;
            se.height = m_Settings.envResolution;
            m_EnvCubemap = TextureCubeMap::Create(se);
            std::fprintf(stderr, "[SkyboxHDR] Env cubemap %p handle=0x%llX %ux%u\n", (void*)m_EnvCubemap.get(), (unsigned long long)m_EnvCubemap->GetHandle(), se.width, se.height);
        }

        {
            TextureSpecification si = s;
            si.width = m_Settings.irradianceResolution;
            si.height = m_Settings.irradianceResolution;
            si.min_filter_param = TextureFilter::LINEAR;
            si.mipmap = false;
            m_IrradianceMap = TextureCubeMap::Create(si);
            std::fprintf(stderr, "[SkyboxHDR] Irradiance cubemap %p handle=0x%llX %ux%u\n", (void*)m_IrradianceMap.get(), (unsigned long long)m_IrradianceMap->GetHandle(), si.width, si.height);
        }

        {
            TextureSpecification sp = s;
            sp.width = m_Settings.prefilterResolution;
            sp.height = m_Settings.prefilterResolution;
            sp.min_filter_param = TextureFilter::LINEAR_MIPMAP_LINEAR;
            sp.mipmap = true;
            m_PrefilterMap = TextureCubeMap::Create(sp);
            std::fprintf(stderr, "[SkyboxHDR] Prefilter cubemap %p handle=0x%llX %ux%u\n", (void*)m_PrefilterMap.get(), (unsigned long long)m_PrefilterMap->GetHandle(), sp.width, sp.height);
        }

        {
            TextureSpecification sb = s;
            sb.width = m_Settings.brdfLUTResolution;
            sb.height = m_Settings.brdfLUTResolution;
            sb.min_filter_param = TextureFilter::LINEAR;
            sb.mipmap = false;
            m_BRDFLUT = Texture2D::Create(sb);
            std::fprintf(stderr, "[SkyboxHDR] BRDF LUT %p handle=0x%llX %ux%u\n", (void*)m_BRDFLUT.get(), (unsigned long long)m_BRDFLUT->GetHandle(), sb.width, sb.height);
        }
    }

    bool SkyboxHDR::GenerateFromHDR(const std::string& hdrPath) {
        std::fprintf(stderr, "[SkyboxHDR] GenerateFromHDR begin: %s\n", hdrPath.c_str());
        stbi_set_flip_vertically_on_load(true);
        int w = 0, h = 0, comp = 0;
        float* data = stbi_loadf(hdrPath.c_str(), &w, &h, &comp, 0);
        if (!data) {
            std::fprintf(stderr, "[SkyboxHDR] Failed to load HDR: %s\n", hdrPath.c_str());
            return false;
        }
        std::fprintf(stderr, "[SkyboxHDR] HDR loaded w=%d h=%d comp=%d\n", w, h, comp);
        if (!m_GL_HDRTex) glGenTextures(1, &m_GL_HDRTex);
        glBindTexture(GL_TEXTURE_2D, m_GL_HDRTex);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB16F, w, h, 0, GL_RGB, GL_FLOAT, data);
        LogGLErr("Upload HDR to GL");
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        LogGLErr("HDR texture params");
        stbi_image_free(data);

        if (!m_GL_FBO) glGenFramebuffers(1, &m_GL_FBO);
        if (!m_GL_RBO) glGenRenderbuffers(1, &m_GL_RBO);
        glBindFramebuffer(GL_FRAMEBUFFER, m_GL_FBO);
        glBindRenderbuffer(GL_RENDERBUFFER, m_GL_RBO);
        glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT24, m_Settings.envResolution, m_Settings.envResolution);
        glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, m_GL_RBO);
        LogGLErr("Create FBO/RBO");
        LogFBOStatus("After FBO setup");
        glBindFramebuffer(GL_FRAMEBUFFER, 0);

        glBindTexture(GL_TEXTURE_CUBE_MAP, (GLuint)m_EnvCubemap->GetHandle());
        for (unsigned i = 0; i < 6; ++i)
            glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X + i, 0, GL_RGB16F,
                m_Settings.envResolution, m_Settings.envResolution, 0, GL_RGB, GL_FLOAT, nullptr);
        glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
        glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        LogGLErr("Env cubemap storage");

        glBindTexture(GL_TEXTURE_CUBE_MAP, (GLuint)m_IrradianceMap->GetHandle());
        for (unsigned i = 0; i < 6; ++i)
            glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X + i, 0, GL_RGB16F,
                m_Settings.irradianceResolution, m_Settings.irradianceResolution, 0, GL_RGB, GL_FLOAT, nullptr);
        glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        LogGLErr("Irradiance cubemap storage");

        glBindTexture(GL_TEXTURE_CUBE_MAP, (GLuint)m_PrefilterMap->GetHandle());
        for (unsigned i = 0; i < 6; ++i)
            glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X + i, 0, GL_RGB16F,
                m_Settings.prefilterResolution, m_Settings.prefilterResolution, 0, GL_RGB, GL_FLOAT, nullptr);
        glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
        glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glGenerateMipmap(GL_TEXTURE_CUBE_MAP);
        LogGLErr("Prefilter cubemap storage");

        m_EquirectToCube->Use();
        bool u1 = m_EquirectToCube->SetUniform("projection", &m_CaptureProj, sizeof(glm::mat4));
        bool ug = m_EquirectToCube->UpdateGlobalState();
        std::fprintf(stderr, "[SkyboxHDR] EquirectToCube uniforms projection=%d UpdateGlobal=%d\n", (int)u1, (int)ug);

        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, m_GL_HDRTex);
        LogGLErr("Bind HDR tex for equirect");

        glViewport(0, 0, m_Settings.envResolution, m_Settings.envResolution);
        glBindFramebuffer(GL_FRAMEBUFFER, m_GL_FBO);
        LogFBOStatus("Before equirect pass");

        for (unsigned i = 0; i < 6; ++i) {
            bool u2 = m_EquirectToCube->SetUniform("view", &m_CaptureViews[i], sizeof(glm::mat4));
            bool ug2 = m_EquirectToCube->UpdateGlobalState();
            std::fprintf(stderr, "[SkyboxHDR] EquirectToCube face=%u view=%d UpdateGlobal=%d\n", i, (int)u2, (int)ug2);

            glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0,
                GL_TEXTURE_CUBE_MAP_POSITIVE_X + i, (GLuint)m_EnvCubemap->GetHandle(), 0);
            LogFBOStatus("Attach env face");
            glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
            m_CubeMesh->draw();
            LogGLErr("Equirect draw face");
        }
        glBindFramebuffer(GL_FRAMEBUFFER, 0);
        glBindTexture(GL_TEXTURE_CUBE_MAP, (GLuint)m_EnvCubemap->GetHandle());
        glGenerateMipmap(GL_TEXTURE_CUBE_MAP);
        LogGLErr("Env mipmap gen");

        m_Irradiance->Use();
        bool iv = m_Irradiance->SetUniform("projection", &m_CaptureProj, sizeof(glm::mat4));
        bool ig = m_Irradiance->UpdateGlobalState();
        std::fprintf(stderr, "[SkyboxHDR] Irradiance uniforms projection=%d UpdateGlobal=%d\n", (int)iv, (int)ig);

        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_CUBE_MAP, (GLuint)m_EnvCubemap->GetHandle());
        LogGLErr("Bind env for irradiance");

        glBindFramebuffer(GL_FRAMEBUFFER, m_GL_FBO);
        glBindRenderbuffer(GL_RENDERBUFFER, m_GL_RBO);
        glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT24,
            m_Settings.irradianceResolution, m_Settings.irradianceResolution);
        glViewport(0, 0, m_Settings.irradianceResolution, m_Settings.irradianceResolution);
        LogFBOStatus("Before irradiance pass");

        for (unsigned i = 0; i < 6; ++i) {
            bool vv = m_Irradiance->SetUniform("view", &m_CaptureViews[i], sizeof(glm::mat4));
            bool vg = m_Irradiance->UpdateGlobalState();
            std::fprintf(stderr, "[SkyboxHDR] Irradiance face=%u view=%d UpdateGlobal=%d\n", i, (int)vv, (int)vg);

            glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0,
                GL_TEXTURE_CUBE_MAP_POSITIVE_X + i, (GLuint)m_IrradianceMap->GetHandle(), 0);
            LogFBOStatus("Attach irradiance face");
            glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
            m_CubeMesh->draw();
            LogGLErr("Irradiance draw face");
        }
        glBindFramebuffer(GL_FRAMEBUFFER, 0);

        m_Prefilter->Use();
        bool pv = m_Prefilter->SetUniform("projection", &m_CaptureProj, sizeof(glm::mat4));
        bool pg = m_Prefilter->UpdateGlobalState();
        std::fprintf(stderr, "[SkyboxHDR] Prefilter uniforms projection=%d UpdateGlobal=%d\n", (int)pv, (int)pg);

        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_CUBE_MAP, (GLuint)m_EnvCubemap->GetHandle());
        LogGLErr("Bind env for prefilter");

        glBindFramebuffer(GL_FRAMEBUFFER, m_GL_FBO);
        const unsigned maxMip = m_Settings.prefilterMipLevels;
        for (unsigned mip = 0; mip < maxMip; ++mip) {
            uint32_t w = (uint32_t)(m_Settings.prefilterResolution * std::pow(0.5f, (float)mip));
            uint32_t h = w;
            glBindRenderbuffer(GL_RENDERBUFFER, m_GL_RBO);
            glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT24, w, h);
            glViewport(0, 0, w, h);
            std::fprintf(stderr, "[SkyboxHDR] Prefilter mip=%u size=%ux%u\n", mip, w, h);

            float roughness = (float)mip / (float)(maxMip - 1);
            bool oru = m_Prefilter->SetUniform("roughness", &roughness, sizeof(float));
            bool oup = m_Prefilter->UpdateObject(nullptr);
            std::fprintf(stderr, "[SkyboxHDR] Prefilter set roughness=%f ok=%d UpdateObject=%d\n", roughness, (int)oru, (int)oup);

            for (unsigned i = 0; i < 6; ++i) {
                bool vv = m_Prefilter->SetUniform("view", &m_CaptureViews[i], sizeof(glm::mat4));
                bool vg = m_Prefilter->UpdateGlobalState();
                std::fprintf(stderr, "[SkyboxHDR] Prefilter face=%u view=%d UpdateGlobal=%d\n", i, (int)vv, (int)vg);

                glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0,
                    GL_TEXTURE_CUBE_MAP_POSITIVE_X + i, (GLuint)m_PrefilterMap->GetHandle(), mip);
                LogFBOStatus("Attach prefilter face");
                glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
                m_CubeMesh->draw();
                LogGLErr("Prefilter draw face");
            }
        }
        glBindFramebuffer(GL_FRAMEBUFFER, 0);

        glBindTexture(GL_TEXTURE_2D, (GLuint)m_BRDFLUT->GetHandle());
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RG16F,
            m_Settings.brdfLUTResolution, m_Settings.brdfLUTResolution,
            0, GL_RG, GL_FLOAT, nullptr);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        LogGLErr("Alloc BRDF LUT");

        glBindFramebuffer(GL_FRAMEBUFFER, m_GL_FBO);
        glBindRenderbuffer(GL_RENDERBUFFER, m_GL_RBO);
        glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT24,
            m_Settings.brdfLUTResolution, m_Settings.brdfLUTResolution);
        glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0,
            GL_TEXTURE_2D, (GLuint)m_BRDFLUT->GetHandle(), 0);
        glViewport(0, 0, m_Settings.brdfLUTResolution, m_Settings.brdfLUTResolution);
        LogFBOStatus("Before BRDF pass");

        m_BRDF->Use();
        bool bgs = m_BRDF->UpdateGlobalState();
        std::fprintf(stderr, "[SkyboxHDR] BRDF UpdateGlobalState=%d\n", (int)bgs);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        m_QuadMesh->draw();
        LogGLErr("BRDF draw");
        glBindFramebuffer(GL_FRAMEBUFFER, 0);

        m_BackgroundMaterial->SetTexture(TextureType::Albedo, m_EnvCubemap.get());
        std::fprintf(stderr, "[SkyboxHDR] Environment map set on background material\n");

        m_IsReady = true;
        std::fprintf(stderr, "[SkyboxHDR] GenerateFromHDR end: ready=1\n");
        return true;
    }
}
