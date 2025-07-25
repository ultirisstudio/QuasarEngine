#pragma once

#include "VulkanTypes.h"

#include "VulkanTexture2D.h"

#include <QuasarEngine/Resources/BasicSkybox.h>
#include <QuasarEngine/Resources/Mesh.h>
#include <QuasarEngine/Resources/Materials/Material.h>
#include <QuasarEngine/Shader/Shader.h>

namespace QuasarEngine
{
	class VulkanBasicSkybox : public BasicSkybox
	{
	public:
		VulkanBasicSkybox();
		~VulkanBasicSkybox() override;

		void Bind() override;
		void Unbind() override;
		void Draw() override;

		void LoadCubemap(const std::array<std::string, 6>& faces) override;

		Shader* GetShader() override;
		Texture2D* GetTexture() override;
		Material* GetMaterial() override;

	private:
		std::unique_ptr<Mesh> cubeMesh;

		std::shared_ptr<Material> material;
		std::shared_ptr<VulkanTexture2D> texture;
		std::shared_ptr<Shader> shader;
	};
}
