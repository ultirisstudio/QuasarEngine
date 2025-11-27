#include "qepch.h"
#include "ShaderGraphCompiler.h"

#include <sstream>
#include <unordered_set>

#include <QuasarEngine/Nodes/Node.h>
#include <QuasarEngine/Nodes/NodeTypes/ConstNode.h>
#include <QuasarEngine/Nodes/NodeTypes/MathNode.h>
#include <QuasarEngine/Nodes/NodeTypes/LogicNode.h>
#include <QuasarEngine/Nodes/NodeTypes/TextureNode.h>
#include <QuasarEngine/Nodes/NodeTypes/UtilityNodes.h>
#include <QuasarEngine/Nodes/NodeTypes/TextureUtilityNodes.h>
#include <QuasarEngine/Nodes/NodeTypes/OutputNodes.h>
#include <QuasarEngine/Nodes/NodeEnumUtils.h>

namespace QuasarEngine
{
    namespace
    {
        struct ShaderExpr
        {
            PortType type = PortType::Unknown;
            std::string code;
        };

        struct BuildContext
        {
            const NodeGraph& graph;
            std::unordered_map<std::string, ShaderExpr> cache;

            std::vector<UsedSampler> samplers;
            std::vector<UsedUniform> uniforms;
        };

        std::string MakeKey(Node::NodeId id, const std::string& port)
        {
            return std::to_string(id) + ":" + port;
        }

        std::string FormatLiteral(PortType type, const std::any& value)
        {
            std::ostringstream oss;

            try
            {
                switch (type)
                {
                case PortType::Float:
                {
                    float v = std::any_cast<float>(value);
                    oss << v << "f";
                    break;
                }
                case PortType::Int:
                {
                    int v = std::any_cast<int>(value);
                    oss << v;
                    break;
                }
                case PortType::Bool:
                {
                    bool v = std::any_cast<bool>(value);
                    oss << (v ? "true" : "false");
                    break;
                }
                case PortType::Vec2:
                {
                    glm::vec2 v = std::any_cast<glm::vec2>(value);
                    oss << "vec2(" << v.x << "f, " << v.y << "f)";
                    break;
                }
                case PortType::Vec3:
                {
                    glm::vec3 v = std::any_cast<glm::vec3>(value);
                    oss << "vec3(" << v.x << "f, " << v.y << "f, " << v.z << "f)";
                    break;
                }
                case PortType::Vec4:
                {
                    glm::vec4 v = std::any_cast<glm::vec4>(value);
                    oss << "vec4(" << v.x << "f, " << v.y << "f, " << v.z << "f, " << v.w << "f)";
                    break;
                }
                default:
                    oss << "0.0";
                    break;
                }
            }
            catch (...)
            {
                oss.str("0.0");
            }

            return oss.str();
        }

        ShaderExpr BuildExprForPort(BuildContext& ctx,
            const std::shared_ptr<Node>& node,
            const std::string& outPortName);

        ShaderExpr GetInputExpr(BuildContext& ctx,
            const std::shared_ptr<Node>& node,
            const std::string& inPortName)
        {
            auto incoming = ctx.graph.GetConnectionsTo(node->GetId());
            for (auto& conn : incoming)
            {
                if (conn->toPort == inPortName)
                {
                    auto fromNode = conn->fromNode.lock();
                    if (!fromNode) break;

                    return BuildExprForPort(ctx, fromNode, conn->fromPort);
                }
            }

            Port* inPort = node->FindInputPort(inPortName);
            if (!inPort || !inPort->value.has_value())
            {
                return { PortType::Unknown, "0.0" };
            }

            ShaderExpr e;
            e.type = inPort->type;
            e.code = FormatLiteral(inPort->type, inPort->value);
            return e;
        }

        void RegisterSamplerIfNeeded(BuildContext& ctx,
            const std::string& name,
            Shader::SamplerType samplerType,
            Shader::ShaderStageFlags stages)
        {
            for (auto& s : ctx.samplers)
            {
                if (s.name == name)
                    return;
            }
            ctx.samplers.push_back(UsedSampler{ name, samplerType, stages });
        }

        ShaderExpr BuildExprForPort(BuildContext& ctx,
            const std::shared_ptr<Node>& node,
            const std::string& outPortName)
        {
            std::string key = MakeKey(node->GetId(), outPortName);
            auto it = ctx.cache.find(key);
            if (it != ctx.cache.end())
                return it->second;

            ShaderExpr result;

            if (auto* constNode = dynamic_cast<ConstNode*>(node.get()))
            {
                PortType t = constNode->GetConstType();
                std::any defVal = constNode->GetDefaultValue();
                result.type = t;
                result.code = FormatLiteral(t, defVal);
            }
            else if (auto* mathNode = dynamic_cast<MathNode*>(node.get()))
            {
                ShaderExpr a = GetInputExpr(ctx, node, "A");
                ShaderExpr b = GetInputExpr(ctx, node, "B");

                const char* opStr = "+";
                switch (mathNode->GetOperation())
                {
                case MathOp::Add: opStr = "+"; break;
                case MathOp::Sub: opStr = "-"; break;
                case MathOp::Mul: opStr = "*"; break;
                case MathOp::Div: opStr = "/"; break;
                default: break;
                }

                result.type = PortType::Float;
                result.code = "(" + a.code + " " + opStr + " " + b.code + ")";
            }
            else if (auto* texNode = dynamic_cast<TextureSampleNode*>(node.get()))
            {
                ShaderExpr uv = GetInputExpr(ctx, node, "UV");

                std::string samplerName = "tex_" + std::to_string(node->GetId());

                RegisterSamplerIfNeeded(
                    ctx,
                    samplerName,
                    Shader::SamplerType::Sampler2D,
                    Shader::StageToBit(Shader::ShaderStageType::Fragment)
                );

                if (outPortName == "Color")
                {
                    result.type = PortType::Vec4;
                    result.code = "texture(" + samplerName + ", " + uv.code + ")";
                }
                else if (outPortName == "R" ||
                    outPortName == "G" ||
                    outPortName == "B" ||
                    outPortName == "A")
                {
                    result.type = PortType::Float;
                    std::string swizzle = "r";
                    if (outPortName == "G") swizzle = "g";
                    else if (outPortName == "B") swizzle = "b";
                    else if (outPortName == "A") swizzle = "a";

                    result.code = "texture(" + samplerName + ", " + uv.code + ")." + swizzle;
                }
                else
                {
                    result.type = PortType::Unknown;
                    result.code = "vec4(0.0)";
                }
            }
            else if (auto* colorNode = dynamic_cast<ColorNode*>(node.get()))
            {
                glm::vec4 c = colorNode->GetColor();
                result.type = PortType::Vec4;
                std::ostringstream oss;
                oss << "vec4(" << c.r << "f, " << c.g << "f, " << c.b << "f, " << c.a << "f)";
                result.code = oss.str();
            }
            else
            {
                const Port* outPort = node->FindOutputPort(outPortName);
                PortType t = outPort ? outPort->type : PortType::Float;

                result.type = t;
                switch (t)
                {
                case PortType::Vec2: result.code = "vec2(0.0)"; break;
                case PortType::Vec3: result.code = "vec3(0.0)"; break;
                case PortType::Vec4: result.code = "vec4(0.0)"; break;
                default: result.code = "0.0"; break;
                }
            }

            ctx.cache[key] = result;
            return result;
        }

        const char* PBR_FRAGMENT_HEADER = R"(
            #version 450 core

            layout(location = 0) in vec2 inTexCoord;
            layout(location = 1) in vec3 inWorldPos;
            layout(location = 2) in vec3 inNormal;
            layout(location = 3) in vec3 inTangent;

            out vec4 outColor;

        )";

        const char* PBR_FRAGMENT_MAIN = R"(

            void main()
            {
                vec4 albedoSample = getAlbedo();
                if (albedoSample.a < 0.5)
                    discard;

                vec3  albedo    = albedoSample.rgb;
                float metallic  = getMetallic();
                float roughness = clamp(getRoughness(), 0.05, 1.0);
                float ao        = getAO();

                vec3 N = getNormal();
                vec3 V = normalize(global_ubo.camera_position - inWorldPos);
                float NdotV = max(dot(N, V), 0.0);

                if (NdotV <= 0.0) {
                    outColor = vec4(0.0);
                    return;
                }

                vec3 R = reflect(-V, N);

                vec3 F0 = mix(vec3(0.04), albedo, metallic);

                PBRContext ctx;
                ctx.N        = N;
                ctx.V        = V;
                ctx.F0       = F0;
                ctx.albedo   = albedo;
                ctx.roughness= roughness;
                ctx.metallic = metallic;
                ctx.NdotV    = NdotV;

                vec3 Lo = vec3(0.0);

                for (int i = 0; i < global_ubo.usePointLight; ++i) {
                    Lo += evaluatePointLight(global_ubo.pointLights[i], ctx, inWorldPos);
                }

                for (int i = 0; i < global_ubo.useDirLight; ++i) {
                    Lo += evaluateDirLight(global_ubo.dirLights[i], ctx);
                }

                vec3 F = fresnelSchlickRoughness(NdotV, F0, roughness);
                vec3 kS = F;
                vec3 kD = (vec3(1.0) - kS) * (1.0 - metallic);

                vec3 F_energy = fresnelSchlickRoughness(NdotV, F0, roughness);
                float avgF    = (F_energy.r + F_energy.g + F_energy.b) * 0.3333333;
                float energyComp = 1.0 + avgF * 0.5;
                kD *= energyComp;

                vec3 irradiance    = texture(irradiance_map, N).rgb;
                vec3 diffuseIBL    = irradiance * albedo;

                float maxLevel = max(float(global_ubo.prefilterLevels - 1), 0.0);
                vec3 prefilteredColor = textureLod(prefilter_map, R, roughness * maxLevel).rgb;
                vec2 brdfSample       = texture(brdf_lut, vec2(NdotV, roughness)).rg;
                vec3 specularIBL      = prefilteredColor * (F * brdfSample.x + brdfSample.y);

                vec3 ambient = (kD * diffuseIBL + specularIBL) * ao;

                vec3 result = ambient + Lo;

                result = result / (result + vec3(1.0));
                outColor = vec4(result, 1.0);
            }
        )";

        const char* PBR_VERTEX_TEMPLATE = R"(
            #version 450 core

            layout(location = 0) in vec3 inPosition;
            layout(location = 1) in vec3 inNormal;
            layout(location = 2) in vec2 inTexCoord;
            layout(location = 3) in vec3 inTangent;

            layout(location = 0) out vec2 outTexCoord;
            layout(location = 1) out vec3 outWorldPos;
            layout(location = 2) out vec3 outNormal;
            layout(location = 3) out vec3 outTangent;

            void main()
            {
                outTexCoord = inTexCoord;

                vec4 worldPos = object_ubo.model * vec4(inPosition, 1.0);
                outWorldPos = worldPos.xyz;

                mat3 normalMatrix = transpose(inverse(mat3(object_ubo.model)));
                outNormal = normalize(normalMatrix * inNormal);

                mat3 model3 = mat3(object_ubo.model);
                outTangent = normalize(model3 * inTangent);

                gl_Position = global_ubo.projection * global_ubo.view * worldPos;
            }
        )";
    }

    ShaderGraphResult ShaderGraphCompiler::CompileMaterialPBR(
        const NodeGraph& graph,
        Node::NodeId materialOutputId,
        const ShaderGraphCompileOptions& options)
    {
        ShaderGraphResult res;
        res.materialOutputId = materialOutputId;

        auto matNodeBase = graph.GetNode(materialOutputId);
        if (!matNodeBase)
            return res;

        auto matNode = std::dynamic_pointer_cast<MaterialOutputNode>(matNodeBase);
        if (!matNode)
            return res;

        BuildContext ctx{ graph };

        ShaderExpr baseColor = GetInputExpr(ctx, matNodeBase, "BaseColor");
        ShaderExpr opacity = GetInputExpr(ctx, matNodeBase, "Opacity");
        ShaderExpr roughness = GetInputExpr(ctx, matNodeBase, "Roughness");
        ShaderExpr metallic = GetInputExpr(ctx, matNodeBase, "Metallic");
        ShaderExpr emissive = GetInputExpr(ctx, matNodeBase, "Emissive");

        std::ostringstream frag;

        frag << PBR_FRAGMENT_HEADER << "\n\n";

        for (auto& s : ctx.samplers)
        {
            if (s.samplerType == Shader::SamplerType::Sampler2D)
                frag << "uniform sampler2D " << s.name << ";\n";
            else if (s.samplerType == Shader::SamplerType::SamplerCube)
                frag << "uniform samplerCube " << s.name << ";\n";
        }
        frag << "\n";

        frag << "vec4 getAlbedo()\n{\n";
        if (baseColor.type == PortType::Vec3)
        {
            frag << "    return vec4(" << baseColor.code << ", 1.0);\n";
        }
        else if (baseColor.type == PortType::Vec4)
        {
            frag << "    return " << baseColor.code << ";\n";
        }
        else
        {
            frag << "    return vec4(1.0);\n";
        }
        frag << "}\n\n";

        frag << "float getRoughness()\n{\n";
        frag << "    return " << roughness.code << ";\n";
        frag << "}\n\n";

        frag << "float getMetallic()\n{\n";
        frag << "    return " << metallic.code << ";\n";
        frag << "}\n\n";

        frag << "float getAO()\n{\n";
        frag << "    return " << opacity.code << ";\n";
        frag << "}\n\n";

        frag << "vec3 getEmissive()\n{\n";
        if (emissive.type == PortType::Vec3)
            frag << "    return " << emissive.code << ";\n";
        else if (emissive.type == PortType::Vec4)
            frag << "    return (" << emissive.code << ").rgb;\n";
        else
            frag << "    return vec3(0.0);\n";
        frag << "}\n\n";

        frag << PBR_FRAGMENT_MAIN << "\n";

        res.fragmentSource = frag.str();
        res.vertexSource = PBR_VERTEX_TEMPLATE;

        res.samplers = ctx.samplers;

        res.uniforms = ctx.uniforms;

        return res;
    }
}