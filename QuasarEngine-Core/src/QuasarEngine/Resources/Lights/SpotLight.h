#pragma once

#include <glm/glm.hpp>

namespace QuasarEngine
{
	struct SpotLight
	{
		glm::vec3 ambient = glm::vec3(0.1f);
		glm::vec3 diffuse = glm::vec3(0.8f);
		glm::vec3 specular = glm::vec3(1.0f);

		glm::vec3 direction = glm::vec3(-1.0f);
		glm::vec3 position = glm::vec3(0.0f);

		float constant = 1.0f;
		float linear = 0.09f;
		float quadratic = 0.032f;

		float cutoff = 10.0f;
		float outerCutoff = 15.0f;
	};
}