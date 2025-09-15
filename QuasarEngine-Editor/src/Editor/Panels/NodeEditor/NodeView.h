#pragma once

#include <imgui/imgui.h>
#include <imgui/imgui_internal.h>

#include <string>
#include <vector>

#include <QuasarEngine/Nodes/Node.h>

namespace QuasarEngine
{
    class NodeEditor;

    class NodeView
    {
    public:
        NodeView(std::shared_ptr<Node> node, ImVec2 pos);

        void Show(bool selected, ImVec2 panOffset, float zoom, ImVec2 canvasPos, NodeEditor* editor);

        ImVec2 GetSize() const { return m_Size; }
        ImVec2 GetPosition() const { return m_Position; }
        void SetPosition(ImVec2 pos) { m_Position = pos; }

        ImVec2 GetLogicalTitleBarOffset(ImVec2 mousePos, ImVec2 panOffset, float zoom, ImVec2 canvasPos) const;
        ImVec2 GetLogicalTitleBarPos(ImVec2 panOffset, float zoom, ImVec2 canvasPos) const;

        ImVec2 GetPortScreenPos(const std::string& portName, bool output, ImVec2 panOffset, float zoom, ImVec2 canvasPos) const;

        Node::NodeId GetId() const { return m_Node->GetId(); }
        std::shared_ptr<Node> GetNode() { return m_Node; }

    private:
        std::shared_ptr<Node> m_Node;
        ImVec2 m_Position;
        ImVec2 m_Size;

        std::vector<ImVec2> m_InputPortPositions;
        std::vector<ImVec2> m_OutputPortPositions;

        void DrawTitleBar(bool selected, ImVec2 panOffset, float zoom, ImVec2 canvasPos);
        void DrawPorts(ImVec2 panOffset, float zoom, ImVec2 canvasPos, NodeEditor* editor);
    };
}