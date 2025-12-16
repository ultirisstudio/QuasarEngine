#include "qepch.h"
#include "ParticleEffectSerializer.h"

#include <fstream>
#include <stdexcept>

#include <yaml-cpp/yaml.h>
#include <glm/glm.hpp>

#include "ParticleEffect.h"

namespace QuasarEngine
{
    static inline YAML::Emitter& operator<<(YAML::Emitter& out, const glm::vec3& v)
    {
        out << YAML::Flow << YAML::BeginSeq << v.x << v.y << v.z << YAML::EndSeq;
        return out;
    }

    static inline YAML::Emitter& operator<<(YAML::Emitter& out, const glm::vec4& v)
    {
        out << YAML::Flow << YAML::BeginSeq << v.r << v.g << v.b << v.a << YAML::EndSeq;
        return out;
    }

    static bool ReadVec3(const YAML::Node& n, glm::vec3& out)
    {
        if (!n || !n.IsSequence() || n.size() != 3) return false;
        out.x = n[0].as<float>();
        out.y = n[1].as<float>();
        out.z = n[2].as<float>();
        return true;
    }

    static bool ReadVec4(const YAML::Node& n, glm::vec4& out)
    {
        if (!n || !n.IsSequence() || n.size() != 4) return false;
        out.r = n[0].as<float>();
        out.g = n[1].as<float>();
        out.b = n[2].as<float>();
        out.a = n[3].as<float>();
        return true;
    }

    static const char* ToString(ParticleEmitterShape s)
    {
        switch (s)
        {
        case ParticleEmitterShape::Point:  return "Point";
        case ParticleEmitterShape::Sphere: return "Sphere";
        case ParticleEmitterShape::Box:    return "Box";
        case ParticleEmitterShape::Cone:   return "Cone";
        default:                           return "Point";
        }
    }

    static ParticleEmitterShape ShapeFromString(const std::string& s)
    {
        if (s == "Sphere") return ParticleEmitterShape::Sphere;
        if (s == "Box")    return ParticleEmitterShape::Box;
        if (s == "Cone")   return ParticleEmitterShape::Cone;
        return ParticleEmitterShape::Point;
    }

    static const char* ToString(ParticleEmitterSimulationSpace s)
    {
        switch (s)
        {
        case ParticleEmitterSimulationSpace::World: return "World";
        case ParticleEmitterSimulationSpace::Local: return "Local";
        default:                                    return "World";
        }
    }

    static ParticleEmitterSimulationSpace SpaceFromString(const std::string& s)
    {
        if (s == "Local") return ParticleEmitterSimulationSpace::Local;
        return ParticleEmitterSimulationSpace::World;
    }

    static const char* ToString(ParticleEmitterBlendMode m)
    {
        switch (m)
        {
        case ParticleEmitterBlendMode::Alpha:    return "Alpha";
        case ParticleEmitterBlendMode::Additive: return "Additive";
        case ParticleEmitterBlendMode::Multiply: return "Multiply";
        default:                                 return "Alpha";
        }
    }

    static ParticleEmitterBlendMode BlendFromString(const std::string& s)
    {
        if (s == "Additive") return ParticleEmitterBlendMode::Additive;
        if (s == "Multiply") return ParticleEmitterBlendMode::Multiply;
        return ParticleEmitterBlendMode::Alpha;
    }

    bool ParticleEffectSerializer::Serialize(const std::string& filepath) const
    {
        if (!m_Effect)
            return false;

        YAML::Emitter out;
        out << YAML::BeginMap;
        out << YAML::Key << "ParticleEffect" << YAML::Value << m_Effect->GetName();

        out << YAML::Key << "Emitters" << YAML::Value << YAML::BeginSeq;

        for (const auto& e : m_Effect->GetEmitters())
        {
            const auto& s = e.settings;

            out << YAML::BeginMap;

            out << YAML::Key << "Name" << YAML::Value << s.name;
            out << YAML::Key << "Enabled" << YAML::Value << s.enabled;
            out << YAML::Key << "Loop" << YAML::Value << s.loop;
            out << YAML::Key << "Duration" << YAML::Value << s.duration;

            out << YAML::Key << "SimulationSpace" << YAML::Value << ToString(s.simulationSpace);
            out << YAML::Key << "BlendMode" << YAML::Value << ToString(s.blendMode);

            out << YAML::Key << "MaxParticles" << YAML::Value << s.maxParticles;
            out << YAML::Key << "SpawnRate" << YAML::Value << s.spawnRate;

            out << YAML::Key << "TextureId" << YAML::Value << e.textureId;
            out << YAML::Key << "TexturePath" << YAML::Value << e.texturePath;

            out << YAML::Key << "Shape" << YAML::Value << ToString(s.shape);
            out << YAML::Key << "PositionOffset" << YAML::Value << s.positionOffset;
            out << YAML::Key << "SphereRadius" << YAML::Value << s.sphereRadius;
            out << YAML::Key << "BoxExtents" << YAML::Value << s.boxExtents;
            out << YAML::Key << "ConeAngle" << YAML::Value << s.coneAngle;
            out << YAML::Key << "ConeRadius" << YAML::Value << s.coneRadius;
            out << YAML::Key << "ConeLength" << YAML::Value << s.coneLength;

            out << YAML::Key << "BaseDirection" << YAML::Value << s.baseDirection;
            out << YAML::Key << "SpreadAngleDeg" << YAML::Value << s.spreadAngleDeg;
            out << YAML::Key << "SpeedMin" << YAML::Value << s.speedMin;
            out << YAML::Key << "SpeedMax" << YAML::Value << s.speedMax;
            out << YAML::Key << "VelocityRandomness" << YAML::Value << s.velocityRandomness;

            out << YAML::Key << "LifeMin" << YAML::Value << s.lifeMin;
            out << YAML::Key << "LifeMax" << YAML::Value << s.lifeMax;

            out << YAML::Key << "StartSizeMin" << YAML::Value << s.startSizeMin;
            out << YAML::Key << "StartSizeMax" << YAML::Value << s.startSizeMax;
            out << YAML::Key << "EndSizeMin" << YAML::Value << s.endSizeMin;
            out << YAML::Key << "EndSizeMax" << YAML::Value << s.endSizeMax;

            out << YAML::Key << "SizeOverLifeExponent" << YAML::Value << s.sizeOverLifeExponent;
            out << YAML::Key << "AlphaOverLifeExponent" << YAML::Value << s.alphaOverLifeExponent;

            out << YAML::Key << "RandomRotation" << YAML::Value << s.randomRotation;
            out << YAML::Key << "AngularVelocityMin" << YAML::Value << s.angularVelocityMin;
            out << YAML::Key << "AngularVelocityMax" << YAML::Value << s.angularVelocityMax;

            out << YAML::Key << "ColorStart" << YAML::Value << s.colorStart;
            out << YAML::Key << "ColorEnd" << YAML::Value << s.colorEnd;

            out << YAML::Key << "Gravity" << YAML::Value << s.gravity;
            out << YAML::Key << "LinearDrag" << YAML::Value << s.linearDrag;
            out << YAML::Key << "Wind" << YAML::Value << s.wind;
            out << YAML::Key << "TurbulenceStrength" << YAML::Value << s.turbulenceStrength;
            out << YAML::Key << "TurbulenceFrequency" << YAML::Value << s.turbulenceFrequency;
            out << YAML::Key << "TurbulenceScale" << YAML::Value << s.turbulenceScale;

            out << YAML::Key << "SoftFade" << YAML::Value << s.softFade;

            out << YAML::Key << "Bursts" << YAML::Value << YAML::BeginSeq;
            for (const auto& b : s.bursts)
            {
                out << YAML::BeginMap;
                out << YAML::Key << "Time" << YAML::Value << b.time;
                out << YAML::Key << "Count" << YAML::Value << b.count;
                out << YAML::EndMap;
            }
            out << YAML::EndSeq;

            out << YAML::EndMap;
        }

        out << YAML::EndSeq;
        out << YAML::EndMap;

        std::ofstream fout(filepath, std::ios::binary);
        if (!fout)
            return false;

        fout << out.c_str();
        return true;
    }

    bool ParticleEffectSerializer::Deserialize(const std::string& filepath)
    {
        if (!m_Effect)
            return false;

        YAML::Node data;
        try
        {
            data = YAML::LoadFile(filepath);
        }
        catch (...)
        {
            return false;
        }

        if (!data || !data["ParticleEffect"])
            return false;

        m_Effect->SetName(data["ParticleEffect"].as<std::string>());
        m_Effect->ClearEmitters();

        YAML::Node emitters = data["Emitters"];
        if (!emitters || !emitters.IsSequence())
            return true;

        for (auto emitterNode : emitters)
        {
            auto& asset = m_Effect->AddEmitter();
            auto& s = asset.settings;

            if (emitterNode["Name"])     s.name = emitterNode["Name"].as<std::string>();
            if (emitterNode["Enabled"])  s.enabled = emitterNode["Enabled"].as<bool>();
            if (emitterNode["Loop"])     s.loop = emitterNode["Loop"].as<bool>();
            if (emitterNode["Duration"]) s.duration = emitterNode["Duration"].as<float>();

            if (emitterNode["SimulationSpace"])
                s.simulationSpace = SpaceFromString(emitterNode["SimulationSpace"].as<std::string>());

            if (emitterNode["BlendMode"])
                s.blendMode = BlendFromString(emitterNode["BlendMode"].as<std::string>());

            if (emitterNode["MaxParticles"]) s.maxParticles = emitterNode["MaxParticles"].as<int>();
            if (emitterNode["SpawnRate"])    s.spawnRate = emitterNode["SpawnRate"].as<float>();

            if (emitterNode["TextureId"])   asset.textureId = emitterNode["TextureId"].as<std::string>();
            if (emitterNode["TexturePath"]) asset.texturePath = emitterNode["TexturePath"].as<std::string>();

            if (emitterNode["Shape"])
                s.shape = ShapeFromString(emitterNode["Shape"].as<std::string>());

            if (emitterNode["PositionOffset"])
                ReadVec3(emitterNode["PositionOffset"], s.positionOffset);
            if (emitterNode["SphereRadius"])
                s.sphereRadius = emitterNode["SphereRadius"].as<float>();
            if (emitterNode["BoxExtents"])
                ReadVec3(emitterNode["BoxExtents"], s.boxExtents);
            if (emitterNode["ConeAngle"])
                s.coneAngle = emitterNode["ConeAngle"].as<float>();
            if (emitterNode["ConeRadius"])
                s.coneRadius = emitterNode["ConeRadius"].as<float>();
            if (emitterNode["ConeLength"])
                s.coneLength = emitterNode["ConeLength"].as<float>();

            if (emitterNode["BaseDirection"])
                ReadVec3(emitterNode["BaseDirection"], s.baseDirection);
            if (emitterNode["SpreadAngleDeg"])
                s.spreadAngleDeg = emitterNode["SpreadAngleDeg"].as<float>();
            if (emitterNode["SpeedMin"])
                s.speedMin = emitterNode["SpeedMin"].as<float>();
            if (emitterNode["SpeedMax"])
                s.speedMax = emitterNode["SpeedMax"].as<float>();
            if (emitterNode["VelocityRandomness"])
                s.velocityRandomness = emitterNode["VelocityRandomness"].as<float>();

            if (emitterNode["LifeMin"]) s.lifeMin = emitterNode["LifeMin"].as<float>();
            if (emitterNode["LifeMax"]) s.lifeMax = emitterNode["LifeMax"].as<float>();

            if (emitterNode["StartSizeMin"]) s.startSizeMin = emitterNode["StartSizeMin"].as<float>();
            if (emitterNode["StartSizeMax"]) s.startSizeMax = emitterNode["StartSizeMax"].as<float>();
            if (emitterNode["EndSizeMin"])   s.endSizeMin = emitterNode["EndSizeMin"].as<float>();
            if (emitterNode["EndSizeMax"])   s.endSizeMax = emitterNode["EndSizeMax"].as<float>();

            if (emitterNode["SizeOverLifeExponent"])
                s.sizeOverLifeExponent = emitterNode["SizeOverLifeExponent"].as<float>();
            if (emitterNode["AlphaOverLifeExponent"])
                s.alphaOverLifeExponent = emitterNode["AlphaOverLifeExponent"].as<float>();

            if (emitterNode["RandomRotation"])
                s.randomRotation = emitterNode["RandomRotation"].as<bool>();
            if (emitterNode["AngularVelocityMin"])
                s.angularVelocityMin = emitterNode["AngularVelocityMin"].as<float>();
            if (emitterNode["AngularVelocityMax"])
                s.angularVelocityMax = emitterNode["AngularVelocityMax"].as<float>();

            if (emitterNode["ColorStart"])
                ReadVec4(emitterNode["ColorStart"], s.colorStart);
            if (emitterNode["ColorEnd"])
                ReadVec4(emitterNode["ColorEnd"], s.colorEnd);

            if (emitterNode["Gravity"])
                ReadVec3(emitterNode["Gravity"], s.gravity);
            if (emitterNode["LinearDrag"])
                s.linearDrag = emitterNode["LinearDrag"].as<float>();
            if (emitterNode["Wind"])
                ReadVec3(emitterNode["Wind"], s.wind);
            if (emitterNode["TurbulenceStrength"])
                s.turbulenceStrength = emitterNode["TurbulenceStrength"].as<float>();
            if (emitterNode["TurbulenceFrequency"])
                s.turbulenceFrequency = emitterNode["TurbulenceFrequency"].as<float>();
            if (emitterNode["TurbulenceScale"])
                s.turbulenceScale = emitterNode["TurbulenceScale"].as<float>();

            if (emitterNode["SoftFade"])
                s.softFade = emitterNode["SoftFade"].as<bool>();

            s.bursts.clear();
            YAML::Node burstsNode = emitterNode["Bursts"];
            if (burstsNode && burstsNode.IsSequence())
            {
                for (auto bn : burstsNode)
                {
                    ParticleBurst b{};
                    if (bn["Time"])  b.time = bn["Time"].as<float>();
                    if (bn["Count"]) b.count = bn["Count"].as<int>();
                    s.bursts.push_back(b);
                }
            }
        }

        return true;
    }
}
