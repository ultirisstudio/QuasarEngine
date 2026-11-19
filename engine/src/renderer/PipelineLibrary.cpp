#include <quasar/renderer/PipelineLibrary.h>

#include <quasar/io/EngineResources.h>
#include <quasar/log/Log.h>

#include <glm/vec4.hpp>

#include <algorithm>
#include <array>
#include <optional>
#include <string>
#include <utility>

namespace Quasar
{
    namespace
    {
        PipelineDefinition GetUnlitDefinition(ShaderSpecification shader)
        {
            return {.Shader = std::move(shader),
                    .Uniforms = {{"Color", UniformType::Vec4}},
                    .Textures = {{.Name = "Texture", .Slot = 0}},
                    .SetDefaults = [](Material& material) { material.SetVec4("Color", glm::vec4(1.0f)); }};
        }

        PipelineDefinition GetBlinnPhongDefinition(ShaderSpecification shader)
        {
            return {.Shader = std::move(shader),
                    .Uniforms = {{"Color", UniformType::Vec4}, {"Specular", UniformType::Float}, {"Shininess", UniformType::Float}},
                    .Textures = {{.Name = "Texture", .Slot = 0}},
                    .SetDefaults = [](Material& material)
                    {
                        material.SetVec4("Color", glm::vec4(1.0f));
                        material.SetFloat("Specular", 0.5f);
                        material.SetFloat("Shininess", 32.0f);
                    }};
        }
    }

    Pipeline* PipelineLibrary::Add(std::string name, PipelineDefinition definition)
    {
        if (name.empty())
        {
            QUASAR_ENGINE_ERROR("Cannot add a pipeline without a name");
            return nullptr;
        }
        if (Find(name) != nullptr)
        {
            QUASAR_ENGINE_ERROR("Cannot add the pipeline '{}': the library already has one of this name", name);
            return nullptr;
        }

        Scope<Quasar::Shader> shader = m_Context->CreateShader(definition.Shader);
        if (shader == nullptr)
        {
            QUASAR_ENGINE_ERROR("Cannot add the pipeline '{}': its shader cannot be created", name);
            return nullptr;
        }
        definition.State.Shader = shader.get();
        Scope<Quasar::Pipeline> pipeline = m_Context->CreatePipeline(definition.State);
        if (pipeline == nullptr)
        {
            QUASAR_ENGINE_ERROR("Cannot add the pipeline '{}': it cannot be created", name);
            return nullptr;
        }

        Pipeline* added = pipeline.get();
        m_Entries.push_back({.Name = std::move(name), .Definition = std::move(definition), .Shader = std::move(shader), .Pipeline = std::move(pipeline)});
        return added;
    }

    bool PipelineLibrary::AddDefaults()
    {
        using GetDefinition = PipelineDefinition (*)(ShaderSpecification);
        const std::array<std::pair<std::string_view, GetDefinition>, 2> defaults{{{Unlit, &GetUnlitDefinition}, {BlinnPhong, &GetBlinnPhongDefinition}}};

        bool added = true;
        for (const auto& [name, getDefinition] : defaults)
        {
            std::optional<ShaderSpecification> shader =
                ShaderSpecification::FromFile(GetEngineResourcesDirectory() / "shaders" / (std::string(name) + ".glsl"));
            if (!shader)
            {
                QUASAR_ENGINE_ERROR("Cannot add the pipeline '{}': its shader cannot be read", name);
                added = false;
                continue;
            }
            added = Add(std::string(name), getDefinition(std::move(*shader))) != nullptr && added;
        }
        return added;
    }

    Pipeline* PipelineLibrary::Get(std::string_view name) const
    {
        const Entry* entry = Find(name);
        return entry != nullptr ? entry->Pipeline.get() : nullptr;
    }

    Scope<Material> PipelineLibrary::CreateMaterial(std::string_view name) const
    {
        const Entry* entry = Find(name);
        if (entry == nullptr)
        {
            QUASAR_ENGINE_ERROR("Cannot create a material of the pipeline '{}': the library has none of this name", name);
            return nullptr;
        }

        Scope<Material> material = Material::Create(*m_Context, {.Pipeline = entry->Pipeline.get(),
                                                                 .UniformBinding = entry->Definition.UniformBinding,
                                                                 .Uniforms = entry->Definition.Uniforms,
                                                                 .Textures = entry->Definition.Textures});
        if (material != nullptr && entry->Definition.SetDefaults)
        {
            entry->Definition.SetDefaults(*material);
        }
        return material;
    }

    const PipelineLibrary::Entry* PipelineLibrary::Find(std::string_view name) const
    {
        const auto entry = std::ranges::find(m_Entries, name, &Entry::Name);
        return entry != m_Entries.end() ? &*entry : nullptr;
    }
}
