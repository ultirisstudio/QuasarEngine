#pragma once

#include <glm/glm.hpp>

namespace QuasarEngine {
    constexpr int QE_MAX_BONE_INFLUENCE = 4;

    struct BoneInfo {
        int id = -1;
        glm::mat4 offset{ 1.f };
    };
}