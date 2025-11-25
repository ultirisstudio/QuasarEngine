#pragma once

#include <imgui/imgui.h>
#include <unordered_map>
#include <memory>

#include <QuasarEngine/Nodes/NodeGraph.h>

#include <Editor/Modules/IEditorModule.h>

namespace QuasarEngine
{
    class NodeView;

    class NodeEditor : public IEditorModule
    {
    public:
        NodeEditor(EditorContext& context);
        ~NodeEditor() override;

        void Update(double dt) override;
		void Render() override;
        void RenderUI() override;

        void ShowInspector();

        void AddNode(const std::string& typeName, ImVec2 pos);
        void DeleteNode(Node::NodeId id);

        void StartConnectionDrag(Node::NodeId nodeId, bool output, const std::string& portName, PortType type, ImVec2 screenPos);
        bool IsConnectionDragActive() const { return m_ConnectionDrag.active; }
        bool IsPortCompatible(Node::NodeId nodeId, bool output, size_t portIdx, PortType type) const;

    private:
        std::shared_ptr<NodeGraph> m_NodeGraph;
        std::unordered_map<Node::NodeId, std::unique_ptr<NodeView>> m_NodeViews;

        Node::NodeId m_SelectedNode = 0;
        Node::NodeId m_DraggingNode = 0;
        ImVec2 m_DragOffset{ 0,0 };

        ImVec2 m_CanvasPos;
        ImVec2 m_CanvasSize;

        bool m_Connecting = false;
        Node::NodeId m_ConnectFromNode = 0;
        std::string m_ConnectFromPort;

        ImVec2 m_PanOffset{ 0,0 };
        float m_Zoom = 1.0f;

        struct ConnectionDragState {
            bool active = false;
            Node::NodeId fromNode = 0;
            bool fromOutput = false;
            std::string fromPortName;
            PortType portType = PortType::Unknown;
            ImVec2 dragStartScreenPos;
        };
        ConnectionDragState m_ConnectionDrag;

        void ShowGraph();
        void HandleEvents(bool hoveringCanvas, bool activeCanvas);
        void DrawConnectionCurve(const ImVec2& from, const ImVec2& to, bool selected = false);

        NodeView* GetNodeView(Node::NodeId id);
    };
}