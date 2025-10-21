#pragma once

#include <memory>
#include <string>
#include <glm/mat4x4.hpp>

#include <QuasarEngine/Resources/Texture2D.h>
#include <QuasarEngine/Resources/TextureCubeMap.h>
#include <QuasarEngine/Shader/Shader.h>
#include <QuasarEngine/Resources/Materials/Material.h>
#include <QuasarEngine/Resources/Mesh.h>

namespace QuasarEngine
{
    class SkyboxHDR {
    public:
        struct Settings {
            uint32_t envResolution = 1024;
            uint32_t irradianceResolution = 32;
            uint32_t prefilterResolution = 1024;
            uint32_t prefilterMipLevels = 5;
            uint32_t brdfLUTResolution = 512;
        };

    public:
        static std::shared_ptr<SkyboxHDR> Create(const Settings& s = Settings());

        ~SkyboxHDR();

        bool LoadFromHDR(const std::string& hdrPath);

        void Draw(const glm::mat4& view, const glm::mat4& projection);

        void Bind();
        void Unbind();

        Shader* GetShader()   const { return m_BackgroundShader.get(); }
        Material* GetMaterial() const { return m_BackgroundMaterial.get(); }

        TextureCubeMap* GetEnvironmentMap() const { return m_EnvCubemap.get(); }
        TextureCubeMap* GetIrradianceMap()  const { return m_IrradianceMap.get(); }
        TextureCubeMap* GetPrefilterMap()   const { return m_PrefilterMap.get(); }
        Texture2D* GetBRDFLUT()        const { return m_BRDFLUT.get(); }

    private:
        explicit SkyboxHDR(const Settings& s);

        void CreateShaders();
        void CreateMeshes();
        void CreateTargets();

        bool GenerateFromHDR(const std::string& hdrPath);
        void BuildCaptureViews();

    private:
        Settings m_Settings;

        std::shared_ptr<Shader> m_EquirectToCube;
        std::shared_ptr<Shader> m_Irradiance;
        std::shared_ptr<Shader> m_Prefilter;
        std::shared_ptr<Shader> m_BRDF;
        std::shared_ptr<Shader> m_BackgroundShader;

        std::shared_ptr<Mesh> m_CubeMesh;
        std::shared_ptr<Mesh> m_QuadMesh;

        std::shared_ptr<TextureCubeMap> m_EnvCubemap;
        std::shared_ptr<TextureCubeMap> m_IrradianceMap;
        std::shared_ptr<TextureCubeMap> m_PrefilterMap;
        std::shared_ptr<Texture2D>      m_BRDFLUT;

        std::unique_ptr<Material> m_BackgroundMaterial;

        glm::mat4 m_CaptureProj{};
        glm::mat4 m_CaptureViews[6]{};

        bool m_IsReady = false;

        unsigned int m_GL_HDRTex = 0;
        unsigned int m_GL_FBO = 0;
        unsigned int m_GL_RBO = 0;
    };
}
