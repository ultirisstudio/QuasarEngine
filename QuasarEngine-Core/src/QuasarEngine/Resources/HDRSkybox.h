#pragma once

#include <glm/glm.hpp>

namespace QuasarEngine
{
    class Texture2D;

    class HDRSkybox {
    public:
        explicit HDRSkybox() = default;
        virtual ~HDRSkybox() = default;

        virtual void LoadFromEquirectangularHDR(const std::string& path) = 0;

        virtual void Draw(const glm::mat4& view, const glm::mat4& projection) = 0;

        virtual Texture2D* GetEnvironmentCubemap() = 0;
        virtual Texture2D* GetIrradianceMap() = 0;
        virtual Texture2D* GetPrefilterMap() = 0;
        virtual Texture2D* GetBRDFLUT() = 0;

        static std::shared_ptr<HDRSkybox> CreateHDRSkybox();
    };
}