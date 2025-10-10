#include "qepch.h"
#include "Animation.h"

#include <cmath>

#include <assimp/scene.h>
#include <assimp/Importer.hpp>
#include <assimp/postprocess.h>

#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtx/quaternion.hpp>

namespace QuasarEngine
{
    static inline glm::vec3 ToGlm(const aiVector3D& v) { return { v.x, v.y, v.z }; }
    static inline glm::quat ToGlm(const aiQuaternion& q) { return { q.w, q.x, q.y, q.z }; }

    static inline float scaleFactor(float t0, float t1, float t)
    {
        const float denom = (t1 - t0);
        if (denom <= 1e-6f) return 0.0f;
        return (t - t0) / denom;
    }

    template <class Key>
    static inline int FindKeyIndex(const std::vector<Key>& keys, float t)
    {
        if (keys.size() <= 1) return 0;

        auto it = std::upper_bound(
            keys.begin(), keys.end(), t,
            [](float v, const Key& k) { return v < k.time; }
        );
        int hi = static_cast<int>(it - keys.begin());
        int idx = std::max(0, std::min(hi - 1, static_cast<int>(keys.size()) - 2));
        return idx;
    }

    glm::mat4 Channel::Sample(float t) const
    {
        glm::vec3 T(0.0f), S(1.0f);
        glm::quat R(1, 0, 0, 0);

        if (!positions.empty())
        {
            if (positions.size() == 1) T = positions[0].value;
            else {
                int i0 = FindKeyIndex(positions, t);
                int i1 = i0 + 1;
                float a = scaleFactor(positions[i0].time, positions[i1].time, t);
                T = glm::mix(positions[i0].value, positions[i1].value, a);
            }
        }

        if (!rotations.empty())
        {
            if (rotations.size() == 1) R = glm::normalize(rotations[0].value);
            else {
                int i0 = FindKeyIndex(rotations, t);
                int i1 = i0 + 1;
                float a = scaleFactor(rotations[i0].time, rotations[i1].time, t);
                R = glm::normalize(glm::slerp(rotations[i0].value, rotations[i1].value, a));
            }
        }

        if (!scales.empty())
        {
            if (scales.size() == 1) S = scales[0].value;
            else {
                int i0 = FindKeyIndex(scales, t);
                int i1 = i0 + 1;
                float a = scaleFactor(scales[i0].time, scales[i1].time, t);
                S = glm::mix(scales[i0].value, scales[i1].value, a);
            }
        }

        return glm::translate(glm::mat4(1.0f), T) * glm::toMat4(R) * glm::scale(glm::mat4(1.0f), S);
    }

    float AnimationClip::WrapTime(float seconds) const
    {
        const float tps = (ticksPerSecond > 0.0f ? ticksPerSecond : 25.0f);
        float t = seconds * tps;
        if (duration > 0.0f) t = std::fmod(t, duration);
        return t;
    }

    float AnimationClip::TimeToTicks(float seconds, bool loop) const
    {
        const float tps = (ticksPerSecond > 0.0f ? ticksPerSecond : 25.0f);
        float t = seconds * tps;

        if (duration > 0.0f)
        {
            if (loop)
            {
                t = std::fmod(std::fmod(t, duration) + duration, duration);
            }
            else
            {
                const float end = std::max(0.0f, duration - 1e-4f);
                if (t < 0.0f) t = 0.0f;
                else if (t > end) t = end;
            }
        }
        return t;
    }

    std::vector<AnimationClip> LoadAnimationClips(const std::string& path)
    {
        std::vector<AnimationClip> clips;

        Assimp::Importer importer;
        const aiScene* scene = importer.ReadFile(
            path,
            aiProcess_JoinIdenticalVertices |
            aiProcess_Triangulate |
            aiProcess_LimitBoneWeights
        );

        if (!scene || !scene->mRootNode || scene->mNumAnimations == 0)
            return clips;

        clips.reserve(scene->mNumAnimations);
        for (unsigned i = 0; i < scene->mNumAnimations; ++i)
        {
            const aiAnimation* a = scene->mAnimations[i];
            AnimationClip clip;
            clip.name = a->mName.length ? a->mName.C_Str() : ("Anim_" + std::to_string(i));
            clip.duration = (float)a->mDuration;
            clip.ticksPerSecond = (a->mTicksPerSecond > 0.0 ? (float)a->mTicksPerSecond : 25.0f);

            for (unsigned c = 0; c < a->mNumChannels; ++c)
            {
                const aiNodeAnim* ch = a->mChannels[c];
                Channel channel;
                channel.nodeName = ch->mNodeName.C_Str();

                channel.positions.reserve(ch->mNumPositionKeys);
                for (unsigned k = 0; k < ch->mNumPositionKeys; ++k)
                    channel.positions.push_back({ ToGlm(ch->mPositionKeys[k].mValue), (float)ch->mPositionKeys[k].mTime });

                channel.rotations.reserve(ch->mNumRotationKeys);
                for (unsigned k = 0; k < ch->mNumRotationKeys; ++k)
                    channel.rotations.push_back({ ToGlm(ch->mRotationKeys[k].mValue), (float)ch->mRotationKeys[k].mTime });

                channel.scales.reserve(ch->mNumScalingKeys);
                for (unsigned k = 0; k < ch->mNumScalingKeys; ++k)
                    channel.scales.push_back({ ToGlm(ch->mScalingKeys[k].mValue), (float)ch->mScalingKeys[k].mTime });

                clip.channels.emplace(channel.nodeName, std::move(channel));
            }

            clips.push_back(std::move(clip));
        }

        return clips;
    }
}
