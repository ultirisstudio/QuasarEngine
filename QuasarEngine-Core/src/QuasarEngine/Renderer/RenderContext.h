#pragma once

#include <array>
#include <memory>
#include <glm/glm.hpp>
#include <QuasarEngine/Resources/Lights/PointLight.h>
#include <QuasarEngine/Resources/Lights/DirectionalLight.h>
#include <QuasarEngine/Resources/SkyboxHDR.h>
#include <QuasarEngine/Scene/Scene.h>
#include <QuasarEngine/Renderer/Framebuffer.h>

namespace QuasarEngine
{
	struct RenderContext
	{
		glm::mat4 view;
		glm::mat4 projection;
		glm::vec3 cameraPosition;

		int numPointLights = 0;
		int numDirLights = 0;
		PointLight* pointLights = nullptr;
		DirectionalLight* dirLights = nullptr;

		SkyboxHDR* skybox = nullptr;

		Scene* scene = nullptr;
	};
}