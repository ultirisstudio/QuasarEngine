#pragma once

#include <memory>
#include <string>

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

#include <QuasarEngine/Renderer/RendererAPI.h>
#include <QuasarEngine/Renderer/DrawMode.h>
#include <QuasarEngine/Renderer/Buffer.h>
#include <QuasarEngine/Renderer/RenderCommand.h>

#include <QuasarEngine/Resources/Mesh.h>
#include <QuasarEngine/Resources/Texture2D.h>
#include <QuasarEngine/Resources/TextureCubeMap.h>
#include <QuasarEngine/Renderer/Framebuffer.h>
#include <QuasarEngine/Shader/Shader.h>

namespace QuasarEngine
{
    class SkyboxHDR
    {
    public:
        struct Settings {
<<<<<<< HEAD
<<<<<<< HEAD
            std::string hdrPath = "Assets/HDR/kloofendal_48d_partly_cloudy_puresky_4k.hdr";
=======
            std::string hdrPath = "Assets/HDR/NightSkyHDRI008_16K_HDR.hdr";
>>>>>>> parent of 6e4f8d6 (Update)
=======
            std::string hdrPath = "Assets/HDR/kloofendal_48d_partly_cloudy_puresky_4k.hdr";
>>>>>>> parent of 97a81a4 (Update)
            uint32_t envRes = 1024;
            uint32_t irradianceRes = 32;
            uint32_t prefilterRes = 1024;
            uint32_t brdfRes = 512;
			uint32_t prefilterMipLevels = 5;
        };

        explicit SkyboxHDR(const Settings& s = {});
        ~SkyboxHDR() = default;

        void Draw(const glm::mat4& view, const glm::mat4& projection);

        const std::shared_ptr<TextureCubeMap>& GetEnvironmentMap() const { return m_EnvCubemap; }
        const std::shared_ptr<TextureCubeMap>& GetIrradianceMap()  const { return m_IrradianceMap; }
        const std::shared_ptr<TextureCubeMap>& GetPrefilterMap()   const { return m_PrefilterMap; }
        const std::shared_ptr<Texture2D>& GetBrdfLUT()        const { return m_BrdfLUT; }

        void BindCubeMap(int unit = 0)       const { if (m_EnvCubemap)     m_EnvCubemap->Bind(unit); }
        void BindIrradianceMap(int unit = 0) const { if (m_IrradianceMap)  m_IrradianceMap->Bind(unit); }
        void BindPrefilterMap(int unit = 0)  const { if (m_PrefilterMap)   m_PrefilterMap->Bind(unit); }
        void BindBrdfLUT(int unit = 0)       const { if (m_BrdfLUT)        m_BrdfLUT->Bind(unit); }

		Settings& GetSettings() { return m_Settings; }

    private:
        static const char* ExtFor(RendererAPI::API api, Shader::ShaderStageType s);
        static uint32_t    CalcMipCount(uint32_t w, uint32_t h);

        void CreateMeshes();
        void CreateShaders();
        void LoadHDR(const std::string& hdrPath);

        void CreateTargets();
        void BuildEnvironment();
        void BuildIrradiance();
        void BuildPrefilter();
        void BuildBrdfLUT();

        Settings m_Settings{};

        std::shared_ptr<Mesh>   m_CubeMesh;
        std::shared_ptr<Mesh>   m_QuadMesh;

        std::shared_ptr<Shader> m_EquirectangularToCubemapShader;
        std::shared_ptr<Shader> m_IrradianceShader;
        std::shared_ptr<Shader> m_BackgroundShader;
        std::shared_ptr<Shader> m_PrefilterShader;
        std::shared_ptr<Shader> m_BrdfShader;

        std::shared_ptr<Texture2D>      m_HDRTexture;
        std::shared_ptr<TextureCubeMap> m_EnvCubemap;
        std::shared_ptr<TextureCubeMap> m_IrradianceMap;
        std::shared_ptr<TextureCubeMap> m_PrefilterMap;
        std::shared_ptr<Texture2D>      m_BrdfLUT;

        std::shared_ptr<Framebuffer>    m_FBO;

        glm::mat4 m_CaptureProjection{};
        glm::mat4 m_CaptureViews[6]{};
    };
}
