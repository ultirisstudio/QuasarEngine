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

        ImVec2 GetSize() const { return size_; }
        ImVec2 GetPosition() const { return position_; }
        void SetPosition(ImVec2 pos) { position_ = pos; }

        ImVec2 GetLogicalTitleBarOffset(ImVec2 mousePos, ImVec2 panOffset, float zoom, ImVec2 canvasPos) const;
        ImVec2 GetLogicalTitleBarPos(ImVec2 panOffset, float zoom, ImVec2 canvasPos) const;

        ImVec2 GetPortScreenPos(const std::string& portName, bool output, ImVec2 panOffset, float zoom, ImVec2 canvasPos) const;

        Node::NodeId GetId() const { return node_->GetId(); }
        std::shared_ptr<Node> GetNode() { return node_; }

    private:
        std::shared_ptr<Node> node_;
        ImVec2 position_;
        ImVec2 size_;

        std::vector<ImVec2> inputPortPositions_;
        std::vector<ImVec2> outputPortPositions_;

        void DrawTitleBar(bool selected, ImVec2 panOffset, float zoom, ImVec2 canvasPos);
        void DrawPorts(ImVec2 panOffset, float zoom, ImVec2 canvasPos, NodeEditor* editor);
    };
}