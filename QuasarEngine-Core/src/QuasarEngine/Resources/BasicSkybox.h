#pragma once

#include <array>
#include <string>
#include <memory>

#include <QuasarEngine/Shader/Shader.h>

namespace QuasarEngine
{
	class Material;

	class BasicSkybox
	{
	public:
		explicit BasicSkybox() = default;
		virtual ~BasicSkybox() = default;

		virtual void Bind() = 0;
		virtual void Unbind() = 0;

		virtual void Draw() = 0;

		virtual void LoadCubemap(const std::array<std::string, 6>& faces) = 0;

		virtual Shader* GetShader() = 0;
		virtual Texture2D* GetTexture() = 0;
		virtual Material* GetMaterial() = 0;

		static std::shared_ptr<BasicSkybox> CreateBasicSkybox();
	};
}