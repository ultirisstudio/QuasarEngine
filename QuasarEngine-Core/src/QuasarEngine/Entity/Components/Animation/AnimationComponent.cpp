#include "qepch.h"
#include "AnimationComponent.h"

#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <QuasarEngine/Asset/AssetManager.h>
#include <glm/gtx/quaternion.hpp>

namespace QuasarEngine
{
    static inline void DecomposeTRS(const glm::mat4& m, glm::vec3& T, glm::quat& R, glm::vec3& S) {
        T = glm::vec3(m[3]);
        glm::vec3 c0(m[0]), c1(m[1]), c2(m[2]);
        S = { glm::length(c0), glm::length(c1), glm::length(c2) };
        if (S.x) c0 /= S.x; if (S.y) c1 /= S.y; if (S.z) c2 /= S.z;
        R = glm::quat_cast(glm::mat3(c0, c1, c2));
    }
    static inline glm::mat4 ComposeTRS(const glm::vec3& T, const glm::quat& R, const glm::vec3& S) {
        return glm::translate(glm::mat4(1), T) * glm::toMat4(R) * glm::scale(glm::mat4(1), S);
    }

    AnimationComponent::AnimationComponent(std::string modelAssetId)
        : m_ModelAssetId(std::move(modelAssetId)) {
    }

    void AnimationComponent::SetModelAssetId(std::string id)
    {
        m_ModelAssetId = std::move(id);
        m_Model.reset();
        m_FinalBoneMatrices.clear();
    }

    void AnimationComponent::SetClips(std::vector<AnimationClip> clips)
    {
        m_Clips = std::move(clips);
        m_CurrentClip = (size_t)-1;
        m_Playing = false;
        m_TimeSec = 0.0;
    }

    void AnimationComponent::Play(size_t clipIndex, bool loop, float speed)
    {
        if (clipIndex >= m_Clips.size()) return;
        m_CurrentClip = clipIndex;
        m_Loop = loop;
        m_Speed = speed;
        m_TimeSec = 0.0;
        m_Playing = true;
    }

    void AnimationComponent::Stop()
    {
        m_Playing = false;
        m_TimeSec = 0.0;
    }

    void AnimationComponent::EnsureModel()
    {
        if (!m_Model.expired()) return;
        if (m_ModelAssetId.empty()) return;
        if (!AssetManager::Instance().isAssetLoaded(m_ModelAssetId)) return;
        m_Model = AssetManager::Instance().getAsset<Model>(m_ModelAssetId);
    }

    void AnimationComponent::EnsureBuffers()
    {
        auto sp = m_Model.lock();
        if (!sp) return;
        const int count = sp->GetBoneCount();
        if ((int)m_FinalBoneMatrices.size() != count)
            m_FinalBoneMatrices.assign(count > 0 ? count : 1, glm::mat4(1.0f));
    }

    void AnimationComponent::Update(double dtSeconds)
    {
        EnsureModel();
        auto sp = m_Model.lock();
        if (!sp) return;

        EnsureBuffers();
        if (!m_Playing || m_CurrentClip == (size_t)-1) return;

        const AnimationClip& clip = m_Clips[m_CurrentClip];

        m_TimeSec += dtSeconds * (double)m_Speed;
        const float tTicks = clip.WrapTime((float)m_TimeSec);

        std::fill(m_FinalBoneMatrices.begin(), m_FinalBoneMatrices.end(), glm::mat4(1.0f));

        std::string effectiveRootBone;
        if (m_InPlace)
        {
            if (!m_RootMotionBone.empty())
            {
                effectiveRootBone = m_RootMotionBone;
            }
            else
            {
                static const char* kCandidates[] = {
                    "Hips", "hips",
                    "mixamorig:Hips", "mixamorig:hips",
                    "Root", "root",
                    "Armature", "armature"
                };

                const auto& bim = sp->GetBoneInfoMap();
                
                for (const char* n : kCandidates)
                {
                    if (bim.find(n) != bim.end()) { effectiveRootBone = n; break; }
                }
                
                if (effectiveRootBone.empty() && !bim.empty())
                    effectiveRootBone = bim.begin()->first;
                
                if (effectiveRootBone.empty() && sp->GetRoot())
                    effectiveRootBone = sp->GetRoot()->name;
            }
        }

        ComputePoseRecursive(sp->GetRoot(), glm::mat4(1.0f), tTicks, m_InPlace, effectiveRootBone);

        if (!m_Loop && m_TimeSec * clip.ticksPerSecond >= clip.duration)
            m_Playing = false;
    }

    void AnimationComponent::ComputePoseRecursive(
        const ModelNode* node,
        const glm::mat4& parent,
        float animTimeTicks,
        bool inPlace,
        const std::string& rootBoneName)
    {
        if (!node) return;

        glm::mat4 local = node->localTransform;
        if (m_CurrentClip != (size_t)-1) {
            const AnimationClip& clip = m_Clips[m_CurrentClip];
            if (auto it = clip.channels.find(node->name); it != clip.channels.end())
                local = it->second.Sample(animTimeTicks);
        }

        if (inPlace && !rootBoneName.empty() && node->name == rootBoneName) {
            glm::vec3 T, S; glm::quat R;
            DecomposeTRS(local, T, R, S);

            T = glm::vec3(0.0f);
            // T.x = 0.0f; T.z = 0.0f;

            local = ComposeTRS(T, R, S);
        }

        glm::mat4 global = parent * local;

        if (auto sp = m_Model.lock()) {
            const auto& map = sp->GetBoneInfoMap();
            if (auto bit = map.find(node->name); bit != map.end()) {
                const int idx = bit->second.id;
                const glm::mat4& offset = bit->second.offset;
                if (idx >= 0 && idx < (int)m_FinalBoneMatrices.size()) {
                    m_FinalBoneMatrices[(size_t)idx] = sp->GetGlobalInverse() * global * offset;
                }
            }
        }

        for (const auto& ch : node->children)
            ComputePoseRecursive(ch.get(), global, animTimeTicks, inPlace, rootBoneName);
    }
}