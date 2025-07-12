#include "NodeView.h"

#include "NodeEnumUtils.h"
#include "NodeEditor.h"

#include <algorithm>

namespace QuasarEngine
{
    static const ImVec2 NODE_DEFAULT_SIZE = ImVec2(180, 0);

    NodeView::NodeView(std::shared_ptr<Node> node, ImVec2 pos)
        : node_(node), position_(pos), size_(NODE_DEFAULT_SIZE) {
    }

    void NodeView::Show(bool selected, ImVec2 panOffset, float zoom, ImVec2 canvasPos, NodeEditor* editor)
    {
        ImDrawList* drawList = ImGui::GetWindowDrawList();
        size_t nPorts = std::max(node_->GetInputPorts().size(), node_->GetOutputPorts().size());
        float nodeHeight = 40.0f + 24.0f * (float)std::max(nPorts, size_t(1));
        size_ = ImVec2(NODE_DEFAULT_SIZE.x, nodeHeight);
        ImVec2 dispPos = position_ * zoom + panOffset + canvasPos;
        ImVec2 dispSize = size_ * zoom;
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
        ImVec2 dispPos = position_ * zoom + panOffset + canvasPos;
        ImVec2 titleBarSize = ImVec2(size_.x * zoom, 28.0f * zoom);
        ImU32 color = selected ? IM_COL32(220, 200, 80, 255) : IM_COL32(70, 70, 95, 255);
        drawList->AddRectFilled(dispPos, dispPos + titleBarSize, color, 8.0f, ImDrawFlags_RoundCornersTop);
        drawList->AddText(dispPos + ImVec2(12.0f * zoom, 7.0f * zoom), IM_COL32(255, 255, 255, 255), node_->GetTypeName().c_str());
    }

    void NodeView::DrawPorts(ImVec2 panOffset, float zoom, ImVec2 canvasPos, NodeEditor* editor)
    {
        ImDrawList* drawList = ImGui::GetWindowDrawList();

        float portRadius = 7.0f * zoom;
        float portSpacing = 24.0f * zoom;

        inputPortPositions_.clear();
        outputPortPositions_.clear();

        const auto& inputs = node_->GetInputPorts();
        const auto& outputs = node_->GetOutputPorts();

        for (size_t i = 0; i < inputs.size(); ++i)
        {
            ImVec2 basePos = position_ * zoom + panOffset + canvasPos;
            ImVec2 portPos = basePos + ImVec2(0, 36.0f * zoom + portSpacing * i);
            ImVec2 circleCenter = portPos + ImVec2(portRadius + 2.0f * zoom, portRadius);

            ImU32 color = GetPortColor(inputs[i].type);
            if (editor && editor->IsConnectionDragActive() && editor->IsPortCompatible(node_->GetId(), false, i, inputs[i].type))
                color = IM_COL32(60, 255, 80, 255);

            drawList->AddCircleFilled(circleCenter, portRadius, color);
            drawList->AddText(portPos + ImVec2(18.0f * zoom, -4.0f * zoom), IM_COL32(200, 200, 220, 255), inputs[i].name.c_str());
            inputPortPositions_.push_back(circleCenter);

            if (ImGui::IsMouseHoveringRect(circleCenter - ImVec2(portRadius, portRadius), circleCenter + ImVec2(portRadius, portRadius)) &&
                ImGui::IsMouseClicked(0))
            {
                if (editor) editor->StartConnectionDrag(node_->GetId(), false, inputs[i].name, inputs[i].type, circleCenter);
            }
        }

        for (size_t i = 0; i < outputs.size(); ++i)
        {
            ImVec2 basePos = position_ * zoom + panOffset + canvasPos;
            ImVec2 portPos = basePos + ImVec2(size_.x * zoom - portRadius * 2.0f - 4.0f * zoom, 36.0f * zoom + portSpacing * i);
            ImVec2 circleCenter = portPos + ImVec2(portRadius, portRadius);

            ImU32 color = GetPortColor(outputs[i].type);
            if (editor && editor->IsConnectionDragActive() && editor->IsPortCompatible(node_->GetId(), true, i, outputs[i].type))
                color = IM_COL32(60, 255, 80, 255);

            drawList->AddCircleFilled(circleCenter, portRadius, color);
            ImVec2 textSize = ImGui::CalcTextSize(outputs[i].name.c_str());
            drawList->AddText(portPos - ImVec2(textSize.x + 8.0f * zoom, 4.0f * zoom), IM_COL32(220, 220, 180, 255), outputs[i].name.c_str());
            outputPortPositions_.push_back(circleCenter);

            if (ImGui::IsMouseHoveringRect(circleCenter - ImVec2(portRadius, portRadius), circleCenter + ImVec2(portRadius, portRadius)) &&
                ImGui::IsMouseClicked(0))
            {
                if (editor) editor->StartConnectionDrag(node_->GetId(), true, outputs[i].name, outputs[i].type, circleCenter);
            }
        }
    }

    ImVec2 NodeView::GetPortScreenPos(const std::string& portName, bool output, ImVec2 panOffset, float zoom, ImVec2 canvasPos) const
    {
        const auto& ports = output ? node_->GetOutputPorts() : node_->GetInputPorts();
        for (size_t i = 0; i < ports.size(); ++i)
        {
            if (ports[i].name == portName)
            {
                if (output)
                {
                    ImVec2 basePos = position_ * zoom + panOffset + canvasPos;
                    float portRadius = 7.0f * zoom;
                    float portSpacing = 24.0f * zoom;
                    ImVec2 portPos = basePos + ImVec2(size_.x * zoom - portRadius * 2.0f - 4.0f * zoom, 36.0f * zoom + portSpacing * i);
                    return portPos + ImVec2(portRadius, portRadius);
                }
                else
                {
                    ImVec2 basePos = position_ * zoom + panOffset + canvasPos;
                    float portRadius = 7.0f * zoom;
                    float portSpacing = 24.0f * zoom;
                    ImVec2 portPos = basePos + ImVec2(0, 36.0f * zoom + portSpacing * i);
                    return portPos + ImVec2(portRadius + 2.0f * zoom, portRadius);
                }
            }
        }
        return position_ * zoom + panOffset + canvasPos;
    }

    ImVec2 NodeView::GetLogicalTitleBarOffset(ImVec2 mousePos, ImVec2 panOffset, float zoom, ImVec2 canvasPos) const
    {
        ImVec2 titleBarPos = position_ * zoom + panOffset + canvasPos;
        return (mousePos - titleBarPos) / zoom;
    }

    ImVec2 NodeView::GetLogicalTitleBarPos(ImVec2 panOffset, float zoom, ImVec2 canvasPos) const
    {
        return position_ * zoom + panOffset + canvasPos;
    }
}
