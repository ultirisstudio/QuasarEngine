#include "qepch.h"
#include "AnimationComponent.h"

#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <QuasarEngine/Asset/AssetManager.h>

namespace QuasarEngine {

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
        float tTicks = clip.WrapTime((float)m_TimeSec);

        std::fill(m_FinalBoneMatrices.begin(), m_FinalBoneMatrices.end(), glm::mat4(1.0f));

        ComputePoseRecursive(sp->GetRoot(), glm::mat4(1.0f), tTicks);

        if (!m_Loop && m_TimeSec * clip.ticksPerSecond >= clip.duration) {
            m_Playing = false;
        }
    }

    void AnimationComponent::ComputePoseRecursive(const ModelNode* node, const glm::mat4& parent, float animTimeTicks)
    {
        if (!node) return;

        glm::mat4 local = node->localTransform;
        if (m_CurrentClip != (size_t)-1) {
            const AnimationClip& clip = m_Clips[m_CurrentClip];
            auto it = clip.channels.find(node->name);
            if (it != clip.channels.end())
                local = it->second.Sample(animTimeTicks);
        }

        glm::mat4 global = parent * local;

        auto sp = m_Model.lock();
        if (sp) {
            const auto& map = sp->GetBoneInfoMap();
            auto bit = map.find(node->name);
            if (bit != map.end()) {
                const int idx = bit->second.id;
                const glm::mat4& offset = bit->second.offset;
                if (idx >= 0 && idx < (int)m_FinalBoneMatrices.size())
                    m_FinalBoneMatrices[(size_t)idx] = sp->GetGlobalInverse() * global * offset;
            }
        }

        for (const auto& ch : node->children)
            ComputePoseRecursive(ch.get(), global, animTimeTicks);
    }
}