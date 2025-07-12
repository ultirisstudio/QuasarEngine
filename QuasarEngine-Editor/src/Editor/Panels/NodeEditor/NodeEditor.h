#pragma once

#include <imgui/imgui.h>
#include <unordered_map>
#include <memory>

#include <QuasarEngine/Nodes/NodeGraph.h>

namespace QuasarEngine
{
    class NodeView;

    class NodeEditor
    {
    public:
        NodeEditor();
        ~NodeEditor();

        void OnImGuiRender();

        void ShowInspector();

        void AddNode(const std::string& typeName, ImVec2 pos);
        void DeleteNode(Node::NodeId id);

        void StartConnectionDrag(Node::NodeId nodeId, bool output, const std::string& portName, PortType type, ImVec2 screenPos);
        bool IsConnectionDragActive() const { return connectionDrag_.active; }
        bool IsPortCompatible(Node::NodeId nodeId, bool output, size_t portIdx, PortType type) const;

    private:
        std::shared_ptr<NodeGraph> nodeGraph_;
        std::unordered_map<Node::NodeId, std::unique_ptr<NodeView>> nodeViews_;

        Node::NodeId selectedNode_ = 0;
        Node::NodeId draggingNode_ = 0;
        ImVec2 dragOffset_{ 0,0 };

        ImVec2 canvasPos;
        ImVec2 canvasSize;

        bool connecting_ = false;
        Node::NodeId connectFromNode_ = 0;
        std::string connectFromPort_;

        ImVec2 panOffset_{ 0,0 };
        float zoom_ = 1.0f;

        struct ConnectionDragState {
            bool active = false;
            Node::NodeId fromNode = 0;
            bool fromOutput = false;
            std::string fromPortName;
            PortType portType = PortType::Unknown;
            ImVec2 dragStartScreenPos;
        };
        ConnectionDragState connectionDrag_;

        void ShowGraph();
        void HandleEvents(bool hoveringCanvas, bool activeCanvas);
        void DrawConnectionCurve(const ImVec2& from, const ImVec2& to, bool selected = false);

        NodeView* GetNodeView(Node::NodeId id);
    };
}