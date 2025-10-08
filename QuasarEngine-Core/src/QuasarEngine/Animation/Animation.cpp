#include "qepch.h"
#include "Animation.h"

#include <assimp/scene.h>
#include <assimp/Importer.hpp>
#include <assimp/postprocess.h>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <glm/gtx/quaternion.hpp>

namespace QuasarEngine
{
    static inline glm::vec3 ToGlm(const aiVector3D& v) { return { v.x, v.y, v.z }; }
    static inline glm::quat ToGlm(const aiQuaternion& q) { return { q.w, q.x, q.y, q.z }; }

    static float scaleFactor(float t0, float t1, float t)
    {
        const float denom = (t1 - t0);
        if (denom <= 1e-6f) return 0.0f;
        return (t - t0) / denom;
    }

    glm::mat4 Channel::Sample(float t) const
    {
        glm::vec3 T(0.0f);
        if (positions.empty()) { }
        else if (positions.size() == 1) T = positions[0].value;
        else {
            int idx = 0;
            while (idx + 1 < (int)positions.size() && t >= positions[idx + 1].time) ++idx;
            const int i1 = std::min(idx + 1, (int)positions.size() - 1);
            const float a = scaleFactor(positions[idx].time, positions[i1].time, t);
            T = glm::mix(positions[idx].value, positions[i1].value, a);
        }

        glm::quat R(1, 0, 0, 0);
        if (rotations.empty()) { }
        else if (rotations.size() == 1) R = glm::normalize(rotations[0].value);
        else {
            int idx = 0;
            while (idx + 1 < (int)rotations.size() && t >= rotations[idx + 1].time) ++idx;
            const int i1 = std::min(idx + 1, (int)rotations.size() - 1);
            const float a = scaleFactor(rotations[idx].time, rotations[i1].time, t);
            R = glm::normalize(glm::slerp(rotations[idx].value, rotations[i1].value, a));
        }

        glm::vec3 S(1.0f);
        if (scales.empty()) { }
        else if (scales.size() == 1) S = scales[0].value;
        else {
            int idx = 0;
            while (idx + 1 < (int)scales.size() && t >= scales[idx + 1].time) ++idx;
            const int i1 = std::min(idx + 1, (int)scales.size() - 1);
            const float a = scaleFactor(scales[idx].time, scales[i1].time, t);
            S = glm::mix(scales[idx].value, scales[i1].value, a);
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

    std::vector<AnimationClip> LoadAnimationClips(const std::string& path)
    {
        std::vector<AnimationClip> clips;

        Assimp::Importer importer;
        const aiScene* scene = importer.ReadFile(
            path,
            aiProcess_JoinIdenticalVertices | aiProcess_Triangulate | aiProcess_LimitBoneWeights
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
