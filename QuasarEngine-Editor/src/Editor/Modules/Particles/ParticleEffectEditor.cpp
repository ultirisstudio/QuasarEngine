#include "qepch.h"
#include "ParticleEffectEditor.h"

#include <QuasarEngine/Resources/Particles/ParticleEffect.h>

namespace QuasarEngine
{
    ParticleEffectEditor::ParticleEffectEditor(EditorContext& context) : IEditorModule(context)
    {
    }

    ParticleEffectEditor::~ParticleEffectEditor()
    {
    }

    void ParticleEffectEditor::RenderUI()
    {
        ImGui::Begin("Particle Editor", nullptr);

        if (!m_Effect)
        {
            ImGui::TextDisabled("No ParticleEffect loaded.");
            ImGui::End();
            return;
        }

        {
            char nameBuf[256];
            std::snprintf(nameBuf, sizeof(nameBuf), "%s", m_Effect->GetName().c_str());
            if (ImGui::InputText("Effect Name", nameBuf, sizeof(nameBuf)))
            {
                m_Effect->SetName(nameBuf);
            }
        }

        ImGui::Separator();

        ImGui::BeginChild("EmittersList", ImVec2(200, 0), true);
        RenderEmitterList();
        ImGui::EndChild();

        ImGui::SameLine();

        ImGui::BeginChild("EmitterInspector", ImVec2(0, 0), false);
        RenderEmitterInspector();

        ImGui::Separator();
        RenderPreview();
        ImGui::EndChild();

        ImGui::End();
    }

    void ParticleEffectEditor::RenderEmitterList()
    {
        auto& emitters = m_Effect->GetEmitters();

        ImGui::TextUnformatted("Emitters");
        ImGui::Separator();

        for (int i = 0; i < (int)emitters.size(); ++i)
        {
            auto& e = emitters[i];
            bool selected = (m_SelectedEmitter == i);

            ImGui::PushID(i);
            if (ImGui::Selectable(e.settings.name.c_str(), selected))
            {
                m_SelectedEmitter = i;
            }
            ImGui::PopID();
        }

        ImGui::Spacing();
        if (ImGui::Button("+ Add Emitter", ImVec2(-1, 0)))
        {
            auto& e = m_Effect->AddEmitter("NewEmitter");
            m_SelectedEmitter = (int)m_Effect->GetEmitterCount() - 1;

            e.settings.baseDirection = { 0.0f, 1.0f, 0.0f };
            e.settings.spreadAngleDeg = 20.0f;
            e.settings.lifeMin = 1.0f;
            e.settings.lifeMax = 2.0f;
            e.settings.spawnRate = 20.0f;
        }

        if (m_SelectedEmitter >= 0 &&
            m_SelectedEmitter < (int)emitters.size())
        {
            if (ImGui::Button("Remove Selected", ImVec2(-1, 0)))
            {
                m_Effect->RemoveEmitter(m_SelectedEmitter);
                if (m_SelectedEmitter >= (int)m_Effect->GetEmitterCount())
                    m_SelectedEmitter = (int)m_Effect->GetEmitterCount() - 1;
            }
        }
    }

    void ParticleEffectEditor::RenderEmitterInspector()
    {
        auto* emitter = m_Effect->GetEmitter(
            m_SelectedEmitter >= 0 ? (std::size_t)m_SelectedEmitter : 0
        );

        if (!emitter)
        {
            ImGui::TextDisabled("No emitter selected.");
            return;
        }

        auto& s = emitter->settings;

        if (ImGui::CollapsingHeader("General", ImGuiTreeNodeFlags_DefaultOpen))
        {
            ImGui::Checkbox("Enabled", &s.enabled);
            ImGui::SameLine();
            ImGui::Checkbox("Loop", &s.loop);

            ImGui::DragFloat("Duration (0 = infinite)", &s.duration, 0.1f, 0.0f, 60.0f);

            char nameBuf[128];
            std::snprintf(nameBuf, sizeof(nameBuf), "%s", s.name.c_str());
            if (ImGui::InputText("Emitter Name", nameBuf, sizeof(nameBuf)))
            {
                s.name = nameBuf;
            }

            int space = (s.simulationSpace == ParticleEmitterSimulationSpace::World) ? 0 : 1;
            const char* spaceLabels[] = { "World", "Local" };
            if (ImGui::Combo("Simulation Space", &space, spaceLabels, 2))
            {
                s.simulationSpace = (space == 0)
                    ? ParticleEmitterSimulationSpace::World
                    : ParticleEmitterSimulationSpace::Local;
            }

            int blend = 0;
            switch (s.blendMode)
            {
            case ParticleEmitterBlendMode::Alpha:    blend = 0; break;
            case ParticleEmitterBlendMode::Additive: blend = 1; break;
            case ParticleEmitterBlendMode::Multiply: blend = 2; break;
            }
            const char* blendLabels[] = { "Alpha", "Additive", "Multiply" };
            if (ImGui::Combo("Blend Mode", &blend, blendLabels, 3))
            {
                if (blend == 1) s.blendMode = ParticleEmitterBlendMode::Additive;
                else if (blend == 2) s.blendMode = ParticleEmitterBlendMode::Multiply;
                else s.blendMode = ParticleEmitterBlendMode::Alpha;
            }

            ImGui::DragInt("Max Particles", &s.maxParticles, 1, 1, 100000);
        }

        if (ImGui::CollapsingHeader("Texture", ImGuiTreeNodeFlags_DefaultOpen))
        {
            ImGui::InputText("Texture ID", emitter->textureId.data(), (int)emitter->textureId.capacity() + 1);
            ImGui::InputText("Texture Path", emitter->texturePath.data(), (int)emitter->texturePath.capacity() + 1);
            ImGui::TextDisabled("À adapter : utiliser ton Content Browser / drag&drop.");
        }

        if (ImGui::CollapsingHeader("Emission", ImGuiTreeNodeFlags_DefaultOpen))
        {
            ImGui::DragFloat("Spawn rate (pps)", &s.spawnRate, 0.1f, 0.0f, 2000.0f);

            if (ImGui::TreeNode("Bursts"))
            {
                int toRemove = -1;
                for (int i = 0; i < (int)s.bursts.size(); ++i)
                {
                    auto& b = s.bursts[i];
                    ImGui::PushID(i);
                    ImGui::DragFloat("Time", &b.time, 0.01f, 0.0f, 60.0f);
                    ImGui::DragInt("Count", &b.count, 1, 0, 100000);
                    ImGui::SameLine();
                    if (ImGui::SmallButton("X"))
                        toRemove = i;
                    ImGui::Separator();
                    ImGui::PopID();
                }
                if (toRemove >= 0)
                    s.bursts.erase(s.bursts.begin() + toRemove);

                if (ImGui::SmallButton("+ Add Burst"))
                {
                    s.bursts.push_back({ 0.0f, 10 });
                }

                ImGui::TreePop();
            }
        }

        if (ImGui::CollapsingHeader("Shape & Position", ImGuiTreeNodeFlags_DefaultOpen))
        {
            int shape = 0;
            switch (s.shape)
            {
            case ParticleEmitterShape::Point:  shape = 0; break;
            case ParticleEmitterShape::Sphere: shape = 1; break;
            case ParticleEmitterShape::Box:    shape = 2; break;
            case ParticleEmitterShape::Cone:   shape = 3; break;
            }

            const char* shapeLabels[] = { "Point", "Sphere", "Box", "Cone" };
            if (ImGui::Combo("Shape", &shape, shapeLabels, 4))
            {
                s.shape = static_cast<ParticleEmitterShape>(shape);
            }

            ImGui::DragFloat3("Position Offset", &s.positionOffset.x, 0.01f);

            if (s.shape == ParticleEmitterShape::Sphere)
            {
                ImGui::DragFloat("Sphere Radius", &s.sphereRadius, 0.01f, 0.0f, 1000.0f);
            }
            else if (s.shape == ParticleEmitterShape::Box)
            {
                ImGui::DragFloat3("Box Extents", &s.boxExtents.x, 0.01f, 0.0f, 1000.0f);
            }
            else if (s.shape == ParticleEmitterShape::Cone)
            {
                ImGui::DragFloat("Cone Angle", &s.coneAngle, 0.1f, 0.0f, 180.0f);
                ImGui::DragFloat("Cone Radius", &s.coneRadius, 0.01f, 0.0f, 1000.0f);
                ImGui::DragFloat("Cone Length", &s.coneLength, 0.01f, 0.0f, 1000.0f);
            }
        }

        if (ImGui::CollapsingHeader("Direction & Velocity", ImGuiTreeNodeFlags_DefaultOpen))
        {
            ImGui::DragFloat3("Base Direction", &s.baseDirection.x, 0.01f, -1.0f, 1.0f);
            ImGui::DragFloat("Spread Angle (deg)", &s.spreadAngleDeg, 0.1f, 0.0f, 180.0f);

            ImGui::DragFloat("Speed Min", &s.speedMin, 0.01f, 0.0f, 1000.0f);
            ImGui::DragFloat("Speed Max", &s.speedMax, 0.01f, 0.0f, 1000.0f);

            ImGui::DragFloat("Velocity randomness", &s.velocityRandomness, 0.01f, 0.0f, 10.0f);
        }

        if (ImGui::CollapsingHeader("Life & Size", ImGuiTreeNodeFlags_DefaultOpen))
        {
            ImGui::DragFloat("Life Min", &s.lifeMin, 0.01f, 0.0f, 1000.0f);
            ImGui::DragFloat("Life Max", &s.lifeMax, 0.01f, 0.0f, 1000.0f);

            ImGui::DragFloat("Start Size Min", &s.startSizeMin, 0.01f, 0.0f, 1000.0f);
            ImGui::DragFloat("Start Size Max", &s.startSizeMax, 0.01f, 0.0f, 1000.0f);
            ImGui::DragFloat("End Size Min", &s.endSizeMin, 0.01f, 0.0f, 1000.0f);
            ImGui::DragFloat("End Size Max", &s.endSizeMax, 0.01f, 0.0f, 1000.0f);

            ImGui::DragFloat("Size over life exponent", &s.sizeOverLifeExponent, 0.01f, 0.1f, 5.0f);
        }

        if (ImGui::CollapsingHeader("Rotation", ImGuiTreeNodeFlags_DefaultOpen))
        {
            ImGui::Checkbox("Random rotation", &s.randomRotation);
            ImGui::DragFloat("Angular vel min", &s.angularVelocityMin, 0.01f, -100.0f, 100.0f);
            ImGui::DragFloat("Angular vel max", &s.angularVelocityMax, 0.01f, -100.0f, 100.0f);
        }

        if (ImGui::CollapsingHeader("Color & Alpha", ImGuiTreeNodeFlags_DefaultOpen))
        {
            ImGui::ColorEdit4("Color Start", &s.colorStart.x);
            ImGui::ColorEdit4("Color End", &s.colorEnd.x);

            ImGui::DragFloat("Alpha over life exponent", &s.alphaOverLifeExponent, 0.01f, 0.1f, 5.0f);
        }

        if (ImGui::CollapsingHeader("Simulation", ImGuiTreeNodeFlags_DefaultOpen))
        {
            ImGui::DragFloat3("Gravity", &s.gravity.x, 0.1f);
            ImGui::DragFloat("Linear drag", &s.linearDrag, 0.01f, 0.0f, 10.0f);

            ImGui::DragFloat3("Wind", &s.wind.x, 0.1f);

            ImGui::DragFloat("Turbulence strength", &s.turbulenceStrength, 0.01f, 0.0f, 10.0f);
            ImGui::DragFloat("Turbulence frequency", &s.turbulenceFrequency, 0.01f, 0.0f, 20.0f);
            ImGui::DragFloat("Turbulence scale", &s.turbulenceScale, 0.01f, 0.0f, 10.0f);

            ImGui::Checkbox("Soft fade", &s.softFade);
        }
    }

    void ParticleEffectEditor::RenderPreview()
    {
        ImGui::TextUnformatted("Preview");
        ImGui::Separator();

        if (ImGui::Button(m_Playing ? "Pause" : "Play"))
            m_Playing = !m_Playing;
        ImGui::SameLine();
        if (ImGui::Button("Restart"))
            m_PreviewTime = 0.0f;
        ImGui::SameLine();
        ImGui::Checkbox("Loop", &m_LoopPreview);

        ImGui::DragFloat("Time scale", &m_TimeScale, 0.01f, 0.1f, 4.0f);

        ImGui::BeginChild("PreviewCanvas", ImVec2(0, 200), true);
        ImGui::TextDisabled("TODO: hook this child area to your 3D preview rendering.");
        ImGui::EndChild();
    }
}
