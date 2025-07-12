#pragma once

#include <glm/glm.hpp>

namespace QuasarEngine
{
    struct alignas(16) DirectionalLight
    {
        glm::vec3 direction = glm::vec3(0.0f);
        float _pad0 = 0.0f;
        glm::vec3 color = glm::vec3(1.0f);
    	float power = 10.0f;
        float _pad1[4] = { 0.0f, 0.0f, 0.0f, 0.0f };
    };
    static_assert(sizeof(DirectionalLight) == 48, "std140: DirectionalLight doit faire 48 bytes");
}