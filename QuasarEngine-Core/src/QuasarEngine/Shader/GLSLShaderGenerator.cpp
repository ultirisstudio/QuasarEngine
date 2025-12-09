#include "qepch.h"

#include <sstream>
#include <unordered_map>

#include <QuasarEngine/Shader/GLSLShaderGenerator.h>

namespace QuasarEngine
{
    namespace
    {
        std::string GlslTypeFromShaderIOType(Shader::ShaderIOType type)
        {
            switch (type)
            {
            case Shader::ShaderIOType::Float: return "float";
            case Shader::ShaderIOType::Vec2:  return "vec2";
            case Shader::ShaderIOType::Vec3:  return "vec3";
            case Shader::ShaderIOType::Vec4:  return "vec4";

            case Shader::ShaderIOType::Int:   return "int";
            case Shader::ShaderIOType::IVec2: return "ivec2";
            case Shader::ShaderIOType::IVec3: return "ivec3";
            case Shader::ShaderIOType::IVec4: return "ivec4";

            case Shader::ShaderIOType::UInt:  return "uint";
            case Shader::ShaderIOType::UVec2: return "uvec2";
            case Shader::ShaderIOType::UVec3: return "uvec3";
            case Shader::ShaderIOType::UVec4: return "uvec4";

            case Shader::ShaderIOType::Mat2:  return "mat2";
            case Shader::ShaderIOType::Mat3:  return "mat3";
            case Shader::ShaderIOType::Mat4:  return "mat4";

            default:                  return "vec4";
            }
        }

        std::string GlslTypeFromShaderUniformType(Shader::ShaderUniformType type)
        {
            switch (type)
            {
            case Shader::ShaderUniformType::Float: return "float";
            case Shader::ShaderUniformType::Vec2:  return "vec2";
            case Shader::ShaderUniformType::Vec3:  return "vec3";
            case Shader::ShaderUniformType::Vec4:  return "vec4";

            case Shader::ShaderUniformType::Int:   return "int";
            case Shader::ShaderUniformType::UInt:  return "uint";

            case Shader::ShaderUniformType::Mat2:  return "mat2";
            case Shader::ShaderUniformType::Mat3:  return "mat3";
            case Shader::ShaderUniformType::Mat4:  return "mat4";

            default:                       return "float";
            }
        }

        const Shader::ShaderIO* FindInputByName(const Shader::ShaderModuleInfo& module, const std::string& name)
        {
            for (const auto& io : module.inputs)
            {
                if (io.isInput && io.name == name)
                    return &io;
            }
            return nullptr;
        }

        const Shader::ShaderIO* FindAnyPositionLikeInput(const Shader::ShaderModuleInfo& module)
        {
            if (auto* io = FindInputByName(module, "a_Position")) return io;
            if (auto* io = FindInputByName(module, "a_Position0")) return io;
            if (auto* io = FindInputByName(module, "inPosition")) return io;
            if (auto* io = FindInputByName(module, "position")) return io;

            for (const auto& io : module.inputs)
            {
                if (!io.isInput) continue;
                if (io.type == Shader::ShaderIOType::Vec3 || io.type == Shader::ShaderIOType::Vec4 ||
                    io.type == Shader::ShaderIOType::Float || io.type == Shader::ShaderIOType::Vec2)
                {
                    return &io;
                }
            }

            return nullptr;
        }
    }

    std::string GLSLShaderGenerator::GetVersionDirective(Shader::ShaderStageType) const
    {
        return "#version 450 core\n";
    }

    void GLSLShaderGenerator::GenerateModules(Shader::ShaderDescription& desc)
    {
        for (auto& module : desc.modules)
        {
            if (!module.path.empty())
                continue;

            switch (module.stage)
            {
            case Shader::ShaderStageType::Vertex:
                module.source = GenerateVertex(desc, module);
                break;

            case Shader::ShaderStageType::Fragment:
                module.source = GenerateFragment(desc, module);
                break;

            case Shader::ShaderStageType::TessControl:
                module.source = GenerateTessControl(desc, module);
                break;

            case Shader::ShaderStageType::TessEval:
                module.source = GenerateTessEval(desc, module);
                break;

            case Shader::ShaderStageType::Geometry:
                module.source = GenerateGeometry(desc, module);
                break;

            case Shader::ShaderStageType::Compute:
                module.source = GenerateCompute(desc, module);
                break;

            default:
                break;
            }
        }
    }

    std::string GLSLShaderGenerator::GenerateVertex(const Shader::ShaderDescription& desc,
        const Shader::ShaderModuleInfo& module) const
    {
        (void)desc;

        std::ostringstream ss;
        ss << GetVersionDirective(Shader::ShaderStageType::Vertex);
        ss << "\n";

        for (const auto& io : module.inputs)
        {
            if (!io.isInput) continue;
            ss << "layout(location = " << io.location << ") in "
                << GlslTypeFromShaderIOType(io.type) << " " << io.name << ";\n";
        }

        for (const auto& io : module.inputs)
        {
            if (io.isInput) continue;
            ss << "layout(location = " << io.location << ") out "
                << GlslTypeFromShaderIOType(io.type) << " " << io.name << ";\n";
        }

        ss << "\nvoid main()\n{\n";

        const Shader::ShaderIO* pos = FindAnyPositionLikeInput(module);
        if (pos)
        {
            if (pos->type == Shader::ShaderIOType::Vec4)
                ss << "    gl_Position = " << pos->name << ";\n";
            else if (pos->type == Shader::ShaderIOType::Vec3)
                ss << "    gl_Position = vec4(" << pos->name << ", 1.0);\n";
            else
                ss << "    gl_Position = vec4(0.0, 0.0, 0.0, 1.0);\n";
        }
        else
        {
            ss << "    gl_Position = vec4(0.0, 0.0, 0.0, 1.0);\n";
        }

        ss << "}\n";
        return ss.str();
    }

    std::string GLSLShaderGenerator::GenerateFragment(const Shader::ShaderDescription& desc,
        const Shader::ShaderModuleInfo& module) const
    {
        (void)desc;

        std::ostringstream ss;
        ss << GetVersionDirective(Shader::ShaderStageType::Fragment);
        ss << "\n";

        for (const auto& io : module.inputs)
        {
            if (!io.isInput) continue;
            ss << "layout(location = " << io.location << ") in "
                << GlslTypeFromShaderIOType(io.type) << " " << io.name << ";\n";
        }

        ss << "layout(location = 0) out vec4 FragColor;\n\n";

        ss << "void main()\n{\n";
        
        ss << "    FragColor = vec4(1.0, 0.0, 1.0, 1.0);\n";
        ss << "}\n";

        return ss.str();
    }

    std::string GLSLShaderGenerator::GenerateTessControl(const Shader::ShaderDescription& desc,
        const Shader::ShaderModuleInfo& module) const
    {
        (void)desc;
        (void)module;

        std::ostringstream ss;
        ss << GetVersionDirective(Shader::ShaderStageType::TessControl);
        ss << "\n";

        ss << "// TODO: Génération de TCS générique\n";
        ss << "layout(vertices = 3) out;\n\n";
        ss << "void main()\n{\n";
        ss << "    gl_out[gl_InvocationID].gl_Position = gl_in[gl_InvocationID].gl_Position;\n";
        ss << "}\n";

        return ss.str();
    }

    std::string GLSLShaderGenerator::GenerateTessEval(const Shader::ShaderDescription& desc,
        const Shader::ShaderModuleInfo& module) const
    {
        (void)desc;
        (void)module;

        std::ostringstream ss;
        ss << GetVersionDirective(Shader::ShaderStageType::TessEval);
        ss << "\n";

        ss << "// TODO: Génération de TES générique\n";
        ss << "layout(triangles, equal_spacing, cw) in;\n\n";
        ss << "void main()\n{\n";
        ss << "    gl_Position = (gl_TessCoord.x * gl_in[0].gl_Position +\n";
        ss << "                   gl_TessCoord.y * gl_in[1].gl_Position +\n";
        ss << "                   gl_TessCoord.z * gl_in[2].gl_Position);\n";
        ss << "}\n";

        return ss.str();
    }

    std::string GLSLShaderGenerator::GenerateGeometry(const Shader::ShaderDescription& desc,
        const Shader::ShaderModuleInfo& module) const
    {
        (void)desc;
        (void)module;

        std::ostringstream ss;
        ss << GetVersionDirective(Shader::ShaderStageType::Geometry);
        ss << "\n";

        ss << "// TODO: Génération de geometry shader générique\n";
        ss << "layout(triangles) in;\n";
        ss << "layout(triangle_strip, max_vertices = 3) out;\n\n";
        ss << "void main()\n{\n";
        ss << "    for (int i = 0; i < 3; ++i)\n";
        ss << "    {\n";
        ss << "        gl_Position = gl_in[i].gl_Position;\n";
        ss << "        EmitVertex();\n";
        ss << "    }\n";
        ss << "    EndPrimitive();\n";
        ss << "}\n";

        return ss.str();
    }

    std::string GLSLShaderGenerator::GenerateCompute(const Shader::ShaderDescription& desc,
        const Shader::ShaderModuleInfo& module) const
    {
        (void)desc;
        (void)module;

        std::ostringstream ss;
        ss << GetVersionDirective(Shader::ShaderStageType::Compute);
        ss << "\n";

        ss << "layout(local_size_x = 8, local_size_y = 8, local_size_z = 1) in;\n\n";
        ss << "void main()\n{\n";
        ss << "    // TODO: compute shader généré\n";
        ss << "}\n";

        return ss.str();
    }
}
