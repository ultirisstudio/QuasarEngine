#include "qepch.h"
#include "ParticleComponentPanel.h"

#include <imgui/imgui.h>
#include <filesystem>

#include <QuasarEngine/Entity/Entity.h>
#include <QuasarEngine/Entity/Components/Particles/ParticleComponent.h>
#include <QuasarEngine/Resources/Texture2D.h>
#include <QuasarEngine/Asset/AssetManager.h>
#include <QuasarEngine/Renderer/Renderer.h>

namespace QuasarEngine
{
    void ParticleComponentPanel::Render(Entity entity)
    {
        if (!entity.HasComponent<ParticleComponent>())
            return;

        auto& pc = entity.GetComponent<ParticleComponent>();

        if (!ImGui::CollapsingHeader("Particle System", ImGuiTreeNodeFlags_DefaultOpen))
            return;

        bool rebuild = false;

        ImGui::SeparatorText("Texture");

        ImGui::Button("Drop texture here", ImVec2(200, 0));
        if (ImGui::BeginDragDropTarget())
        {
            if (const ImGuiPayload* payload = ImGui::AcceptDragDropPayload("CONTENT_BROWSER_ITEM"))
            {
                const wchar_t* wpath = (const wchar_t*)payload->Data;
                std::filesystem::path abs = WeakCanonical(wpath);
                std::string id = BuildAssetIdFromAbs(abs, AssetManager::Instance().getAssetPath());

                pc.SetTexture(id, abs.string());
                rebuild = true;
            }
            ImGui::EndDragDropTarget();
        }

        if (!pc.GetTextureId().empty())
        {
            ImGui::Text("ID: %s", pc.GetTextureId().c_str());
            if (!pc.GetTexturePath().empty())
                ImGui::TextDisabled("%s", pc.GetTexturePath().c_str());

            if (ImGui::SmallButton("Clear texture"))
            {
                pc.ClearTexture();
                rebuild = true;
            }
        }
        else
        {
            ImGui::TextDisabled("Aucune texture assignée");
        }

        ImGui::SeparatorText("Global");

        ImGui::Checkbox("Enabled", &pc.m_Enabled);

        int maxP = pc.m_MaxParticles;
        if (ImGui::DragInt("Max particles", &maxP, 1, 1, 100000))
        {
            pc.m_MaxParticles = std::max(1, maxP);
            rebuild = true;
        }

        ImGui::SeparatorText("Emission");

        ImGui::DragFloat("Spawn rate (pps)", &pc.m_SpawnRate, 0.1f, 0.0f, 1000.0f);

        ImGui::Checkbox("Burst mode", &pc.m_BurstMode);
        if (pc.m_BurstMode)
        {
            ImGui::DragInt("Burst count", &pc.m_BurstCount, 1, 1, 10000);
            ImGui::Checkbox("Loop bursts", &pc.m_Loop);
        }

        ImGui::DragFloat3("Emitter offset", &pc.m_EmitterOffset.x, 0.01f);

        ImGui::SeparatorText("Direction & Shape");

        ImGui::DragFloat3("Base direction", &pc.m_BaseDirection.x, 0.01f, -1.0f, 1.0f);
        ImGui::DragFloat("Spread angle (deg)", &pc.m_SpreadAngleDegrees, 1.0f, 0.0f, 180.0f);

        ImGui::DragFloat("Position spread", &pc.m_PositionSpread, 0.01f, 0.0f, 100.0f);
        ImGui::DragFloat("Velocity spread", &pc.m_VelocitySpread, 0.01f, 0.0f, 10.0f);

        ImGui::SeparatorText("Speed & Life");

        ImGui::DragFloat("Speed min", &pc.m_SpeedMin, 0.01f, 0.0f, 100.0f);
        ImGui::DragFloat("Speed max", &pc.m_SpeedMax, 0.01f, 0.0f, 100.0f);

        ImGui::DragFloat("Life min", &pc.m_LifeMin, 0.01f, 0.0f, 100.0f);
        ImGui::DragFloat("Life max", &pc.m_LifeMax, 0.01f, 0.0f, 100.0f);

        ImGui::SeparatorText("Size");

        ImGui::DragFloat("Start size min", &pc.m_StartSizeMin, 0.01f, 0.0f, 100.0f);
        ImGui::DragFloat("Start size max", &pc.m_StartSizeMax, 0.01f, 0.0f, 100.0f);
        ImGui::DragFloat("End size min", &pc.m_EndSizeMin, 0.01f, 0.0f, 100.0f);
        ImGui::DragFloat("End size max", &pc.m_EndSizeMax, 0.01f, 0.0f, 100.0f);

        ImGui::DragFloat("Size over life exp", &pc.m_SizeOverLifeExponent, 0.01f, 0.1f, 5.0f);

        ImGui::SeparatorText("Rotation");

        ImGui::Checkbox("Random rotation", &pc.m_RandomRotation);
        ImGui::DragFloat("Angular vel min", &pc.m_AngularVelocityMin, 0.01f, -20.0f, 20.0f);
        ImGui::DragFloat("Angular vel max", &pc.m_AngularVelocityMax, 0.01f, -20.0f, 20.0f);

        ImGui::SeparatorText("Colors");

        ImGui::ColorEdit4("Color start", &pc.m_ColorStart.x);
        ImGui::ColorEdit4("Color end", &pc.m_ColorEnd.x);

        ImGui::DragFloat("Alpha over life exp", &pc.m_AlphaOverLifeExponent, 0.01f, 0.1f, 5.0f);

        ImGui::SeparatorText("Simulation");

        ImGui::Checkbox("Override sim", &pc.m_OverrideSimulation);
        if (pc.m_OverrideSimulation)
        {
            ImGui::DragFloat3("Gravity", &pc.m_Gravity.x, 0.1f);
            ImGui::DragFloat("Linear drag", &pc.m_LinearDrag, 0.01f, 0.0f, 10.0f);

            ImGui::DragFloat3("Wind", &pc.m_Wind.x, 0.1f);

            ImGui::DragFloat("Turbulence strength", &pc.m_TurbulenceStrength, 0.01f, 0.0f, 10.0f);
            ImGui::DragFloat("Turbulence freq", &pc.m_TurbulenceFrequency, 0.01f, 0.0f, 20.0f);
            ImGui::DragFloat("Turbulence scale", &pc.m_TurbulenceScale, 0.01f, 0.0f, 10.0f);
        }

        if (rebuild)
            pc.RebuildSystem();
    }
}