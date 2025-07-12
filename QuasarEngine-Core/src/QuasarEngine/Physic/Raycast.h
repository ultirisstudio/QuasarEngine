#pragma once
/*
#include <glm/glm.hpp>
#include <QuasarEngine/Scene/Scene.h>
#include "QuasarEngine/Physic/RaycastCallback.h"

namespace QuasarEngine {

    struct Ray {
        glm::vec3 origin;
        glm::vec3 direction;

        Ray(const glm::vec3& o, const glm::vec3& d) : origin(o), direction(glm::normalize(d)) {}

        glm::vec3 GetPoint(float t) const {
            return origin + direction * t;
        }
    };

    struct RaycastInfo {
        bool Hit = false;
        float Distance = 0.0f;
        glm::vec3 HitPoint = glm::vec3(0.0f);
        glm::vec3 Normal = glm::vec3(0.0f);
        uint32_t Entity = 0;
    };

    class Raycast {
    public:
        static RaycastInfo RaycastAll(Scene* scene, const glm::vec3& origin, const glm::vec3& direction, float maxDistance = 1000.0f);
        static void RaycastWithCallback(Scene* scene, const glm::vec3& origin, const glm::vec3& direction, RaycastCallback* callback, float maxDistance = 1000.0f);

    };

}*/
