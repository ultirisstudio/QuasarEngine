#include "NodeView.h"

#include "NodeEnumUtils.h"
#include "NodeEditor.h"

#include <algorithm>

namespace QuasarEngine
{
    static const ImVec2 NODE_DEFAULT_SIZE = ImVec2(180, 0);

    NodeView::NodeView(std::shared_ptr<Node> node, ImVec2 pos)
        : m_Node(node), m_Position(pos), m_Size(NODE_DEFAULT_SIZE) {
    }

    void NodeView::Show(bool selected, ImVec2 panOffset, float zoom, ImVec2 canvasPos, NodeEditor* editor)
    {
        ImDrawList* drawList = ImGui::GetWindowDrawList();
        size_t nPorts = std::max(m_Node->GetInputPorts().size(), m_Node->GetOutputPorts().size());
        float nodeHeight = 40.0f + 24.0f * (float)std::max(nPorts, size_t(1));
        m_Size = ImVec2(NODE_DEFAULT_SIZE.x, nodeHeight);
        ImVec2 dispPos = m_Position * zoom + panOffset + canvasPos;
        ImVec2 dispSize = m_Size * zoom;
        ImU32 bgColor = selected ? IM_COL32(200, 170, 40, 240) : IM_COL32(45, 45, 50, 240);
        drawList->AddRectFilled(dispPos, dispPos + dispSize, bgColor, 8.0f);
        drawList->AddRect(dispPos, dispPos + dispSize,
            selected ? IM_COL32(255, 220, 60, 255) : IM_COL32(120, 120, 120, 120),
            8.0f, 0, 2.0f);
        DrawTitleBar(selected, panOffset, zoom, canvasPos);
        DrawPorts(panOffset, zoom, canvasPos, editor);
    }

    void NodeView::DrawTitleBar(bool selected, ImVec2 panOffset, float zoom, ImVec2 canvasPos)
    {
        ImDrawList* drawList = ImGui::GetWindowDrawList();
        ImVec2 dispPos = m_Position * zoom + panOffset + canvasPos;
        ImVec2 titleBarSize = ImVec2(m_Size.x * zoom, 28.0f * zoom);
        ImU32 color = selected ? IM_COL32(220, 200, 80, 255) : IM_COL32(70, 70, 95, 255);
        drawList->AddRectFilled(dispPos, dispPos + titleBarSize, color, 8.0f, ImDrawFlags_RoundCornersTop);
        drawList->AddText(dispPos + ImVec2(12.0f * zoom, 7.0f * zoom), IM_COL32(255, 255, 255, 255), m_Node->GetTypeName().c_str());
    }

    void NodeView::DrawPorts(ImVec2 panOffset, float zoom, ImVec2 canvasPos, NodeEditor* editor)
    {
        ImDrawList* drawList = ImGui::GetWindowDrawList();

        float portRadius = 7.0f * zoom;
        float portSpacing = 24.0f * zoom;

        m_InputPortPositions.clear();
        m_OutputPortPositions.clear();

        const auto& inputs = m_Node->GetInputPorts();
        const auto& outputs = m_Node->GetOutputPorts();

        for (size_t i = 0; i < inputs.size(); ++i)
        {
            ImVec2 basePos = m_Position * zoom + panOffset + canvasPos;
            ImVec2 portPos = basePos + ImVec2(0, 36.0f * zoom + portSpacing * i);
            ImVec2 circleCenter = portPos + ImVec2(portRadius + 2.0f * zoom, portRadius);

            ImU32 color = GetPortColor(inputs[i].type);
            if (editor && editor->IsConnectionDragActive() && editor->IsPortCompatible(m_Node->GetId(), false, i, inputs[i].type))
                color = IM_COL32(60, 255, 80, 255);

            drawList->AddCircleFilled(circleCenter, portRadius, color);
            drawList->AddText(portPos + ImVec2(18.0f * zoom, -4.0f * zoom), IM_COL32(200, 200, 220, 255), inputs[i].name.c_str());
            m_InputPortPositions.push_back(circleCenter);

            if (ImGui::IsMouseHoveringRect(circleCenter - ImVec2(portRadius, portRadius), circleCenter + ImVec2(portRadius, portRadius)) &&
                ImGui::IsMouseClicked(0))
            {
                if (editor) editor->StartConnectionDrag(m_Node->GetId(), false, inputs[i].name, inputs[i].type, circleCenter);
            }
        }

        for (size_t i = 0; i < outputs.size(); ++i)
        {
            ImVec2 basePos = m_Position * zoom + panOffset + canvasPos;
            ImVec2 portPos = basePos + ImVec2(m_Size.x * zoom - portRadius * 2.0f - 4.0f * zoom, 36.0f * zoom + portSpacing * i);
            ImVec2 circleCenter = portPos + ImVec2(portRadius, portRadius);

            ImU32 color = GetPortColor(outputs[i].type);
            if (editor && editor->IsConnectionDragActive() && editor->IsPortCompatible(m_Node->GetId(), true, i, outputs[i].type))
                color = IM_COL32(60, 255, 80, 255);

            drawList->AddCircleFilled(circleCenter, portRadius, color);
            ImVec2 textSize = ImGui::CalcTextSize(outputs[i].name.c_str());
            drawList->AddText(portPos - ImVec2(textSize.x + 8.0f * zoom, 4.0f * zoom), IM_COL32(220, 220, 180, 255), outputs[i].name.c_str());
            m_OutputPortPositions.push_back(circleCenter);

            if (ImGui::IsMouseHoveringRect(circleCenter - ImVec2(portRadius, portRadius), circleCenter + ImVec2(portRadius, portRadius)) &&
                ImGui::IsMouseClicked(0))
            {
                if (editor) editor->StartConnectionDrag(m_Node->GetId(), true, outputs[i].name, outputs[i].type, circleCenter);
            }
        }
    }

    ImVec2 NodeView::GetPortScreenPos(const std::string& portName, bool output, ImVec2 panOffset, float zoom, ImVec2 canvasPos) const
    {
        const auto& ports = output ? m_Node->GetOutputPorts() : m_Node->GetInputPorts();
        for (size_t i = 0; i < ports.size(); ++i)
        {
            if (ports[i].name == portName)
            {
                if (output)
                {
                    ImVec2 basePos = m_Position * zoom + panOffset + canvasPos;
                    float portRadius = 7.0f * zoom;
                    float portSpacing = 24.0f * zoom;
                    ImVec2 portPos = basePos + ImVec2(m_Size.x * zoom - portRadius * 2.0f - 4.0f * zoom, 36.0f * zoom + portSpacing * i);
                    return portPos + ImVec2(portRadius, portRadius);
                }
                else
                {
                    ImVec2 basePos = m_Position * zoom + panOffset + canvasPos;
                    float portRadius = 7.0f * zoom;
                    float portSpacing = 24.0f * zoom;
                    ImVec2 portPos = basePos + ImVec2(0, 36.0f * zoom + portSpacing * i);
                    return portPos + ImVec2(portRadius + 2.0f * zoom, portRadius);
                }
            }
        }
        return m_Position * zoom + panOffset + canvasPos;
    }

    ImVec2 NodeView::GetLogicalTitleBarOffset(ImVec2 mousePos, ImVec2 panOffset, float zoom, ImVec2 canvasPos) const
    {
        ImVec2 titleBarPos = m_Position * zoom + panOffset + canvasPos;
        return (mousePos - titleBarPos) / zoom;
    }

    ImVec2 NodeView::GetLogicalTitleBarPos(ImVec2 panOffset, float zoom, ImVec2 canvasPos) const
    {
        return m_Position * zoom + panOffset + canvasPos;
    }
}
