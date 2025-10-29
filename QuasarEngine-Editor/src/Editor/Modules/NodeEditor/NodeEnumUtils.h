#pragma once

#include <string>
#include <imgui/imgui.h>

#include <QuasarEngine/Nodes/Node.h>
#include <QuasarEngine/Nodes/NodeTypes/MathNode.h>
#include <QuasarEngine/Nodes/NodeTypes/LogicNode.h>

namespace QuasarEngine
{
    inline std::string ToString(PortType type)
    {
        switch (type)
        {
        case PortType::Float: return "Float";
        case PortType::Int:   return "Int";
        case PortType::Bool:  return "Bool";
        case PortType::Vec2:  return "Vec2";
        case PortType::Vec3:  return "Vec3";
        case PortType::Vec4:  return "Vec4";
        case PortType::String:  return "String";
        default: return "Unknown";
        }
    }

    inline ImU32 GetPortColor(PortType type)
    {
        switch (type)
        {
        case PortType::Float: return IM_COL32(80, 190, 240, 255);
        case PortType::Int:   return IM_COL32(200, 110, 140, 255);
        case PortType::Bool:  return IM_COL32(240, 210, 60, 255);
        case PortType::Vec2:  return IM_COL32(120, 240, 170, 255);
        case PortType::Vec3:  return IM_COL32(120, 180, 255, 255);
        case PortType::Vec4:  return IM_COL32(190, 130, 255, 255);
        case PortType::String:return IM_COL32(60, 40, 120, 255);
        default:              return IM_COL32(140, 140, 140, 255);
        }
    }

    inline std::string ToString(MathOp op)
    {
        switch (op)
        {
        case MathOp::Add: return "Add";
        case MathOp::Sub: return "Sub";
        case MathOp::Mul: return "Mul";
        case MathOp::Div: return "Div";

        default: return "Unknown";
        }
    }

    inline std::string ToString(LogicOp op)
    {
        switch (op)
        {
        case LogicOp::And: return "And";
        case LogicOp::Or:  return "Or";
        case LogicOp::Not: return "Not";
        case LogicOp::Xor: return "Xor";

        default: return "Unknown";
        }
    }
}
