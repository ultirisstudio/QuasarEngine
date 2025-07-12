#pragma once

#include <QuasarEngine/Resources/HDRSkybox.h>

namespace QuasarEngine
{
    class VulkanTexture2D;
    class VulkanShader;

    class Mesh;

    class VulkanHDRSkybox : public HDRSkybox
    {
    public:
        VulkanHDRSkybox();
        ~VulkanHDRSkybox() override;

        void LoadFromEquirectangularHDR(const std::string& path) override;

        void Draw(const glm::mat4& view, const glm::mat4& projection) override;

        Texture2D* GetEnvironmentCubemap() override;
        Texture2D* GetIrradianceMap() override;
        Texture2D* GetPrefilterMap() override;
        Texture2D* GetBRDFLUT() override;

    private:
        std::unique_ptr<VulkanShader> equirectToCubemapShader;
        std::unique_ptr<VulkanShader> irradianceShader;
        std::unique_ptr<VulkanShader> prefilterShader;
        std::unique_ptr<VulkanShader> brdfShader;
        std::unique_ptr<VulkanShader> backgroundShader;

        std::shared_ptr<VulkanTexture2D> hdrTexture;
        std::shared_ptr<VulkanTexture2D> environmentCubemap;
        std::shared_ptr<VulkanTexture2D> irradianceMap;
        std::shared_ptr<VulkanTexture2D> prefilterMap;
        std::shared_ptr<VulkanTexture2D> brdfLUT;

        std::unique_ptr<Mesh> cubeMesh;
    };
}

/*
void VulkanHDRSkybox::LoadFromEquirectangularHDR(const std::string& path) {
    // 1. Charger HDR dans une image 2D Vulkan (hdrTexture)
    // 2. Créer environmentCubemap (VK_IMAGE_VIEW_TYPE_CUBE)
    // 3. Rendu offscreen :
    //      pour chaque face { set la view matrix, draw avec equirectToCubemapShader }
    // 4. Générer irradianceMap avec irradianceShader et offscreen rendering (idem)
    // 5. Générer prefilterMap avec prefilterShader et mipmapping
    // 6. Générer brdfLUT avec brdfShader et quad fullscreen
}
*/