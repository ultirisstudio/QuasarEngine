#include "NodeView.h"

#include "NodeEnumUtils.h"
#include "NodeEditor.h"

#include <algorithm>

namespace QuasarEngine
{
    static const ImVec2 NODE_DEFAULT_SIZE = ImVec2(220, 0);

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

        auto& inputs = m_Node->GetInputPorts();
        auto& outputs = m_Node->GetOutputPorts();

        float portRadius = 7.0f * zoom;
        float portSpacing = 24.0f * zoom;
        float titleHeight = 28.0f * zoom;
        float topOffset = titleHeight + 8.0f * zoom;

        ImVec2 basePos = m_Position * zoom + panOffset + canvasPos;

        m_InputPortPositions.clear();
        m_OutputPortPositions.clear();

        ImGui::PushID(m_Node->GetId());

        for (size_t i = 0; i < inputs.size(); ++i)
        {
            const auto& port = inputs[i];

            ImVec2 portPos = basePos + ImVec2(4.0f * zoom, topOffset + portSpacing * i);
            ImVec2 circleCenter = portPos + ImVec2(portRadius, portRadius);

            m_InputPortPositions.push_back(circleCenter);

            ImU32 color = GetPortColor(port.type);

            if (editor && editor->IsConnectionDragActive() &&
                editor->IsPortCompatible(m_Node->GetId(), false, i, port.type))
            {
                color = IM_COL32(60, 255, 80, 255);
            }

            drawList->AddCircleFilled(circleCenter, portRadius, color);

            ImVec2 textSize = ImGui::CalcTextSize(port.name.c_str());
            drawList->AddText(
                portPos - ImVec2(textSize.x + 6.0f * zoom, 2.0f * zoom),
                IM_COL32(220, 220, 200, 255),
                port.name.c_str()
            );

            if (editor)
            {
                ImRect hoverRect(
                    circleCenter - ImVec2(portRadius + 4.0f * zoom, portRadius + 4.0f * zoom),
                    circleCenter + ImVec2(portRadius + 4.0f * zoom, portRadius + 4.0f * zoom)
                );

                if (hoverRect.Contains(ImGui::GetIO().MousePos) &&
                    ImGui::IsMouseClicked(ImGuiMouseButton_Left))
                {
                    editor->StartConnectionDrag(m_Node->GetId(), false, port.name, port.type, circleCenter);
                }
            }

            bool isConnected = editor ? editor->IsInputConnected(m_Node->GetId(), port.name) : false;
            if (!isConnected)
            {
                float widgetOffsetX = 40.0f * zoom;
                ImVec2 widgetPos = circleCenter + ImVec2(widgetOffsetX, -ImGui::GetTextLineHeight() * 0.5f);

                ImGui::SetCursorScreenPos(widgetPos);
                ImGui::PushID(static_cast<int>(i));

                std::any& valAny = m_Node->GetInputPortValue(port.name);

                if (port.type == PortType::Float)
                {
                    float v = 0.0f;
                    try { v = std::any_cast<float>(valAny); }
                    catch (...) {}
                    ImGui::PushItemWidth(60.0f * zoom);
                    if (ImGui::DragFloat("##in_float", &v, 0.1f))
                        valAny = v;
                    ImGui::PopItemWidth();
                }
                else if (port.type == PortType::Int)
                {
                    int v = 0;
                    try { v = std::any_cast<int>(valAny); }
                    catch (...) {}
                    ImGui::PushItemWidth(60.0f * zoom);
                    if (ImGui::DragInt("##in_int", &v, 1.0f))
                        valAny = v;
                    ImGui::PopItemWidth();
                }
                else if (port.type == PortType::Bool)
                {
                    bool v = false;
                    try { v = std::any_cast<bool>(valAny); }
                    catch (...) {}
                    if (ImGui::Checkbox("##in_bool", &v))
                        valAny = v;
                }
                else if (port.type == PortType::String)
                {
                    static char buf[128] = {};
                    try
                    {
                        auto s = std::any_cast<std::string>(valAny);
                        strncpy(buf, s.c_str(), sizeof(buf));
                        buf[sizeof(buf) - 1] = '\0';
                    }
                    catch (...) { buf[0] = '\0'; }

                    ImGui::PushItemWidth(100.0f * zoom);
                    if (ImGui::InputText("##in_str", buf, sizeof(buf)))
                        valAny = std::string(buf);
                    ImGui::PopItemWidth();
                }
                else if (port.type == PortType::Vec2)
                {
                    glm::vec2 v(0.0f);
                    try { v = std::any_cast<glm::vec2>(valAny); }
                    catch (...) {}
                    float arr[2] = { v.x, v.y };
                    ImGui::PushItemWidth(120.0f * zoom);
                    if (ImGui::InputFloat2("##in_vec2", arr))
                        valAny = glm::vec2(arr[0], arr[1]);
                    ImGui::PopItemWidth();
                }
                else if (port.type == PortType::Vec3)
                {
                    glm::vec3 v(0.0f);
                    try { v = std::any_cast<glm::vec3>(valAny); }
                    catch (...) {}
                    float arr[3] = { v.x, v.y, v.z };
                    ImGui::PushItemWidth(150.0f * zoom);
                    if (ImGui::InputFloat3("##in_vec3", arr))
                        valAny = glm::vec3(arr[0], arr[1], arr[2]);
                    ImGui::PopItemWidth();
                }
                else if (port.type == PortType::Vec4)
                {
                    glm::vec4 v(0.0f);
                    try { v = std::any_cast<glm::vec4>(valAny); }
                    catch (...) {}
                    float arr[4] = { v.x, v.y, v.z, v.w };
                    ImGui::PushItemWidth(180.0f * zoom);
                    if (ImGui::InputFloat4("##in_vec4", arr))
                        valAny = glm::vec4(arr[0], arr[1], arr[2], arr[3]);
                    ImGui::PopItemWidth();
                }

                ImGui::PopID();
            }
        }

        ImGui::PopID();

        for (size_t i = 0; i < outputs.size(); ++i)
        {
            const auto& port = outputs[i];

            ImVec2 portPos = basePos + ImVec2(m_Size.x * zoom - (portRadius * 2.0f + 4.0f * zoom),
                topOffset + portSpacing * i);
            ImVec2 circleCenter = portPos + ImVec2(portRadius, portRadius);

            m_OutputPortPositions.push_back(circleCenter);

            ImU32 color = GetPortColor(port.type);

            if (editor && editor->IsConnectionDragActive() &&
                editor->IsPortCompatible(m_Node->GetId(), true, i, port.type))
            {
                color = IM_COL32(60, 255, 80, 255);
            }

            drawList->AddCircleFilled(circleCenter, portRadius, color);

            ImVec2 textSize = ImGui::CalcTextSize(port.name.c_str());
            drawList->AddText(
                portPos - ImVec2(textSize.x + 8.0f * zoom, 2.0f * zoom),
                IM_COL32(220, 220, 180, 255),
                port.name.c_str()
            );

            if (editor)
            {
                ImRect hoverRect(
                    circleCenter - ImVec2(portRadius + 4.0f * zoom, portRadius + 4.0f * zoom),
                    circleCenter + ImVec2(portRadius + 4.0f * zoom, portRadius + 4.0f * zoom)
                );

                if (hoverRect.Contains(ImGui::GetIO().MousePos) &&
                    ImGui::IsMouseClicked(ImGuiMouseButton_Left))
                {
                    editor->StartConnectionDrag(m_Node->GetId(), true, port.name, port.type, circleCenter);
                }
            }
        }

        size_t rows = std::max(inputs.size(), outputs.size());
        m_Size.y = (rows > 0 ? topOffset + portSpacing * rows + 8.0f * zoom : titleHeight + 16.0f * zoom) / zoom;
    }

    ImVec2 NodeView::GetPortScreenPos(const std::string& portName, bool output, ImVec2 panOffset, float zoom, ImVec2 canvasPos) const
    {
        const auto& ports = output ? m_Node->GetOutputPorts() : m_Node->GetInputPorts();

        float portRadius = 7.0f * zoom;
        float portSpacing = 24.0f * zoom;
        float titleHeight = 28.0f * zoom;
        float topOffset = titleHeight + 8.0f * zoom;

        ImVec2 basePos = m_Position * zoom + panOffset + canvasPos;

        for (size_t i = 0; i < ports.size(); ++i)
        {
            if (ports[i].name == portName)
            {
                if (output)
                {
                    ImVec2 portPos = basePos + ImVec2(m_Size.x * zoom - (portRadius * 2.0f + 4.0f * zoom),
                        topOffset + portSpacing * i);
                    return portPos + ImVec2(portRadius, portRadius);
                }
                else
                {
                    ImVec2 portPos = basePos + ImVec2(4.0f * zoom, topOffset + portSpacing * i);
                    return portPos + ImVec2(portRadius, portRadius);
                }
            }
        }
        return basePos;
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
