#pragma once

#include <string>
#include <unordered_map>
#include <vector>
#include <glm/glm.hpp>
#include <glm/gtc/quaternion.hpp>

namespace QuasarEngine
{
    struct KeyPosition { glm::vec3 value{}; float time{}; };
    struct KeyRotation { glm::quat value{}; float time{}; };
    struct KeyScale { glm::vec3 value{}; float time{}; };

    struct Channel
    {
        std::string nodeName;
        std::vector<KeyPosition> positions;
        std::vector<KeyRotation> rotations;
        std::vector<KeyScale>    scales;

        glm::mat4 Sample(float t) const;
    };

    struct AnimationClip
    {
        std::string name;
        float duration = 0.0f;
        float ticksPerSecond = 25.0f;
        std::unordered_map<std::string, Channel> channels;

        float WrapTime(float seconds) const;
    };

    std::vector<AnimationClip> LoadAnimationClips(const std::string& path);
}
