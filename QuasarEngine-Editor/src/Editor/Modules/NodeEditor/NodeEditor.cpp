#include "NodeEditor.h"
#include <algorithm>
#include <iostream>

#include <glm/glm.hpp>

#include "NodeRegistry.h"
#include "NodeView.h"
#include "NodeEnumUtils.h"

namespace QuasarEngine
{
	NodeEditor::NodeEditor(EditorContext& context) : IEditorModule(context)
    {
        for (int t = 0; t < static_cast<int>(PortType::COUNT); ++t)
        {
            PortType type = static_cast<PortType>(t);
            if (type == PortType::Unknown) continue;
            //NodeRegistry::Instance().Register("Variable_" + ToString(type), [type](Node::NodeId id) { return std::make_shared<VariableNode>(id, type); });
            std::string name = "Const_" + ToString(type);
            NodeRegistry::Instance().Register(name, [&, type](Node::NodeId id) { return std::make_shared<ConstNode>(id, ToString(type), type, 0.0f); });
        }

        for (int t = 0; t < static_cast<int>(MathOp::COUNT); ++t)
        {
            MathOp mathOp = static_cast<MathOp>(t);
            std::string name = "Math_" + ToString(mathOp);
            NodeRegistry::Instance().Register(name, [&, mathOp](Node::NodeId id) { return std::make_shared<MathNode>(id, ToString(mathOp), mathOp); });
        }

        for (int t = 0; t < static_cast<int>(LogicOp::COUNT); ++t)
        {
            LogicOp logicOp = static_cast<LogicOp>(t);
            std::string name = "Logic_" + ToString(logicOp);
            NodeRegistry::Instance().Register(name, [&, logicOp](Node::NodeId id) { return std::make_shared<LogicNode>(id, ToString(logicOp), logicOp); });
        }

        m_NodeGraph = std::make_shared<NodeGraph>();
    }

    NodeEditor::~NodeEditor() {}

    void NodeEditor::Update(double dt)
    {

    }

    void NodeEditor::Render()
    {

    }

    void NodeEditor::RenderUI()
    {
        ImGui::Begin("Node Editor", nullptr);

        ImVec2 pos = ImGui::GetCursorScreenPos();
        ImVec2 size = ImGui::GetContentRegionAvail();

        if (size.x <= 0 || size.y <= 0)
        {
            ImGui::End();
            return;
        }

        m_Canvas.BeginRegion(pos, size);
        m_Canvas.baseGridStep = 40.0f;

        ImGui::SetCursorScreenPos(m_Canvas.canvasPos +ImVec2(10, 10));
        ImGui::SetItemAllowOverlap();
        bool wantOpenPopup = false;
        if (ImGui::Button("+ Ajouter un noeud", ImVec2(150, 0)))
        {
            wantOpenPopup = true;
            std::cout << "print" << std::endl;
        }

        if (wantOpenPopup)
            ImGui::OpenPopup("AddNodePopup");

        ImGui::SetCursorScreenPos(m_Canvas.canvasPos);

        ImGui::InvisibleButton("##NodeCanvas", ImVec2(m_Canvas.canvasSize.x * 0.7f, m_Canvas.canvasSize.y), ImGuiButtonFlags_MouseButtonLeft | ImGuiButtonFlags_MouseButtonRight | ImGuiButtonFlags_MouseButtonMiddle);
        bool hoveringCanvas = ImGui::IsItemHovered();
        bool activeCanvas = ImGui::IsItemActive();

        ShowGraph();
        HandleEvents(hoveringCanvas, activeCanvas);

        if (hoveringCanvas && ImGui::IsMouseClicked(1))
            ImGui::OpenPopup("AddNodePopup");

        if (ImGui::BeginPopup("AddNodePopup"))
        {
            for (const auto& typeName : NodeRegistry::Instance().GetRegisteredTypes())
            {
                if (ImGui::MenuItem(typeName.c_str()))
                {
                    ImVec2 mouse = ImGui::GetIO().MousePos;
                    AddNode(typeName, m_Canvas.ScreenToCanvas(mouse));
                    ImGui::CloseCurrentPopup();
                }
            }
            ImGui::EndPopup();
        }

        ImGui::SameLine();

        ImGui::BeginChild("Inspector", ImVec2(0, m_Canvas.canvasSize.y), false);
        ShowInspector();
        ImGui::EndChild();

        ImGui::End();
    }

    void NodeEditor::ShowInspector()
    {
        if (!m_SelectedNode)
        {
            ImGui::PushStyleColor(ImGuiCol_Text, IM_COL32(180, 180, 200, 180));
            ImGui::Text("Aucun noeud selectionne");
            ImGui::PopStyleColor();
            return;
        }

        NodeView* view = GetNodeView(m_SelectedNode);
        if (!view)
            return;
        auto node = view->GetNode();

        ImGui::PushFont(ImGui::GetIO().Fonts->Fonts[0]);
        ImGui::TextColored(ImVec4(0.95f, 0.85f, 0.45f, 1.0f), "%s", node->GetTypeName().c_str());
        ImGui::PopFont();

        ImGui::Dummy(ImVec2(0, 6));
        ImGui::Separator();
        ImGui::Dummy(ImVec2(0, 8));

        if (auto* constNode = dynamic_cast<ConstNode*>(node.get()))
        {
            PortType type = constNode->GetOutputPorts()[0].type;
            std::any& valAny = constNode->GetOutputPortValue("Value");

            if (type == PortType::Float)
            {
                float val = 0.0f;
                try { val = std::any_cast<float>(valAny); }
                catch (...) {}
                ImGui::PushItemWidth(120);
                if (ImGui::DragFloat("Valeur##float", &val, 0.1f, -10000.0f, 10000.0f, "%.3f"))
                    valAny = val;
                ImGui::PopItemWidth();
            }
            else if (type == PortType::Int)
            {
                int val = 0;
                try { val = std::any_cast<int>(valAny); }
                catch (...) {}
                ImGui::PushItemWidth(120);
                if (ImGui::InputInt("Valeur##int", &val))
                    valAny = val;
                ImGui::PopItemWidth();
            }
            else if (type == PortType::Bool)
            {
                bool val = false;
                try { val = std::any_cast<bool>(valAny); }
                catch (...) {}
                if (ImGui::Checkbox("Valeur##bool", &val))
                    valAny = val;
            }
            else if (type == PortType::String)
            {
                static char buf[256] = {};
                try { strncpy(buf, std::any_cast<std::string>(valAny).c_str(), sizeof(buf)); }
                catch (...) { buf[0] = '\0'; }
                ImGui::PushItemWidth(180);
                if (ImGui::InputText("Valeur##str", buf, sizeof(buf)))
                    valAny = std::string(buf);
                ImGui::PopItemWidth();
            }
            else if (type == PortType::Vec2)
            {
                glm::vec2 vec(0.0f, 0.0f);
                try { vec = std::any_cast<glm::vec2>(valAny); }
                catch (...) {}
                float v[2] = { vec.x, vec.y };
                ImGui::PushItemWidth(180);
                if (ImGui::InputFloat2("Valeur##vec2", v))
                    valAny = glm::vec2(v[0], v[1]);
                ImGui::PopItemWidth();
            }
            else if (type == PortType::Vec3)
            {
                glm::vec3 vec(0.0f, 0.0f, 0.0f);
                try { vec = std::any_cast<glm::vec3>(valAny); }
                catch (...) {}
                float v[3] = { vec.x, vec.y, vec.z };
                ImGui::PushItemWidth(210);
                if (ImGui::InputFloat3("Valeur##vec3", v))
                    valAny = glm::vec3(v[0], v[1], v[2]);
                ImGui::PopItemWidth();
            }
            else if (type == PortType::Vec4)
            {
                glm::vec4 vec(0.0f, 0.0f, 0.0f, 0.0f);
                try { vec = std::any_cast<glm::vec4>(valAny); }
                catch (...) {}
                float v[4] = { vec.x, vec.y, vec.z, vec.w };
                ImGui::PushItemWidth(250);
                if (ImGui::InputFloat4("Valeur##vec4", v))
                    valAny = glm::vec4(v[0], v[1], v[2], v[3]);
                ImGui::PopItemWidth();
            }
            else
            {
                ImGui::TextDisabled("Type non editable.");
            }
        }
        else
        {
            ImGui::TextDisabled("Ce noeud ne possede pas de valeur modifiable.");
        }

        ImGui::Dummy(ImVec2(0, 14));
        ImGui::Separator();
        ImGui::Dummy(ImVec2(0, 10));
    }

    void NodeEditor::ShowGraph()
    {
        ImDrawList* drawList = ImGui::GetWindowDrawList();

        drawList->PushClipRect(m_Canvas.canvasPos, m_Canvas.canvasPos + m_Canvas.canvasSize, true);
        m_Canvas.baseGridStep = 40.0f;
        m_Canvas.DrawGrid(drawList);

        if (m_ConnectionDrag.active)
        {
            ImVec2 fromPos = m_ConnectionDrag.dragStartScreenPos;
            ImVec2 toPos = ImGui::GetIO().MousePos;
            DrawConnectionCurve(fromPos, toPos, false);
        }

        for (const auto& conn : m_NodeGraph->GetConnections())
        {
            auto fromNode = conn->fromNode.lock();
            auto toNode = conn->toNode.lock();
            if (!fromNode || !toNode) continue;
            auto* fromView = GetNodeView(fromNode->GetId());
            auto* toView = GetNodeView(toNode->GetId());
            if (!fromView || !toView) continue;
            ImVec2 fromPos = fromView->GetPortScreenPos(conn->fromPort, true, m_Canvas.pan, m_Canvas.zoom, m_Canvas.canvasPos);
            ImVec2 toPos = toView->GetPortScreenPos(conn->toPort, false, m_Canvas.pan, m_Canvas.zoom, m_Canvas.canvasPos);
            DrawConnectionCurve(fromPos, toPos);
        }

        for (auto& [id, nodeView] : m_NodeViews)
            nodeView->Show(id == m_SelectedNode, m_Canvas.pan, m_Canvas.zoom, m_Canvas.canvasPos, this);

        ImDrawList* debugDraw = ImGui::GetWindowDrawList();
        if (m_DraggingNode)
        {
            auto* view = GetNodeView(m_DraggingNode);
            if (view)
            {
                ImVec2 titlePos = view->GetPosition() * m_Canvas.zoom + m_Canvas.pan + m_Canvas.canvasPos;
                ImVec2 titleSize = ImVec2(view->GetSize().x * m_Canvas.zoom, 28.0f * m_Canvas.zoom);
                debugDraw->AddRect(titlePos, titlePos + titleSize, IM_COL32(255, 0, 0, 255), 0, 0, 2.0f);
            }
        }

        drawList->PopClipRect();
    }

    void NodeEditor::DrawConnectionCurve(const ImVec2& from, const ImVec2& to, bool selected)
    {
        ImDrawList* drawList = ImGui::GetWindowDrawList();
        float dist = fabsf(to.x - from.x) * 0.5f;
        ImVec2 cp1 = ImVec2(from.x + dist, from.y);
        ImVec2 cp2 = ImVec2(to.x - dist, to.y);
        drawList->AddBezierCubic(from, cp1, cp2, to,
            selected ? IM_COL32(255, 200, 0, 255) : IM_COL32(200, 200, 200, 200),
            3.0f);
    }

    NodeView* NodeEditor::GetNodeView(Node::NodeId id)
    {
        auto it = m_NodeViews.find(id);
        return it != m_NodeViews.end() ? it->second.get() : nullptr;
    }

    void NodeEditor::HandleEvents(bool hoveringCanvas, bool activeCanvas)
    {
        ImGuiIO& io = ImGui::GetIO();

        if (hoveringCanvas && !m_DraggingNode)
        {
            for (auto& [id, nodeView] : m_NodeViews)
            {
                ImVec2 titlePos = nodeView->GetPosition() * m_Canvas.zoom + m_Canvas.pan + m_Canvas.canvasPos;
                ImVec2 titleSize = ImVec2(nodeView->GetSize().x * m_Canvas.zoom, 28.0f * m_Canvas.zoom);
                ImRect titleBar(titlePos, titlePos + titleSize);
                if (ImGui::IsMouseClicked(0) && titleBar.Contains(io.MousePos))
                {
                    m_DraggingNode = id;
                    m_DragOffset = (io.MousePos - titlePos) / m_Canvas.zoom;
                    m_SelectedNode = id;
                    break;
                }
            }
        }
        if (m_DraggingNode)
        {
            if (ImGui::IsMouseDown(0))
            {
                auto* view = GetNodeView(m_DraggingNode);
                if (view)
                {
                    ImVec2 newLogicalPos = (io.MousePos - m_Canvas.pan - m_Canvas.canvasPos) / m_Canvas.zoom - m_DragOffset;
                    view->SetPosition(newLogicalPos);
                }
            }
            else
            {
                m_DraggingNode = 0;
            }
        }

        static bool panning = false;
        static ImVec2 panStart = ImVec2(0, 0);

        if (hoveringCanvas || activeCanvas)
        {
            m_Canvas.HandlePanAndZoom(io, true, 0.25f, 2.5f);
        }

        if (m_ConnectionDrag.active && ImGui::IsMouseReleased(0))
        {
            ImVec2 mouse = ImGui::GetIO().MousePos;
            Node::NodeId bestNode = 0;
            bool bestIsOutput = false;
            std::string bestPortName;

            for (auto& [id, nodeView] : m_NodeViews)
            {
                if (id == m_ConnectionDrag.fromNode) continue;

                const auto& inputs = nodeView->GetNode()->GetInputPorts();
                for (size_t i = 0; i < inputs.size(); ++i)
                {
                    ImVec2 pos = nodeView->GetPortScreenPos(inputs[i].name, false, m_Canvas.pan, m_Canvas.zoom, m_Canvas.canvasPos);
                    float portRadius = 7.0f * m_Canvas.zoom;
                    ImVec2 delta = mouse - pos;
                    float distSqr = delta.x * delta.x + delta.y * delta.y;
                    if (distSqr < (portRadius + 4.0f) * (portRadius + 4.0f) && IsPortCompatible(id, false, i, inputs[i].type))
                    {
                        bestNode = id; bestIsOutput = false; bestPortName = inputs[i].name;
                    }
                }

                const auto& outputs = nodeView->GetNode()->GetOutputPorts();
                for (size_t i = 0; i < outputs.size(); ++i)
                {
                    ImVec2 pos = nodeView->GetPortScreenPos(outputs[i].name, true, m_Canvas.pan, m_Canvas.zoom, m_Canvas.canvasPos);
                    float portRadius = 7.0f * m_Canvas.zoom;
                    ImVec2 delta = mouse - pos;
                    float distSqr = delta.x * delta.x + delta.y * delta.y;
                    if (distSqr < (portRadius + 4.0f) * (portRadius + 4.0f) && IsPortCompatible(id, true, i, outputs[i].type))
                    {
                        bestNode = id; bestIsOutput = true; bestPortName = outputs[i].name;
                    }
                }
            }

            if (!bestPortName.empty())
            {
                Node::NodeId fromNode = m_ConnectionDrag.fromNode;
                std::string fromPort = m_ConnectionDrag.fromPortName;
                Node::NodeId toNode = bestNode;
                std::string toPort = bestPortName;

                if (m_ConnectionDrag.fromOutput && !bestIsOutput)
                    m_NodeGraph->Connect(fromNode, fromPort, toNode, toPort);
                else if (!m_ConnectionDrag.fromOutput && bestIsOutput)
                    m_NodeGraph->Connect(toNode, toPort, fromNode, fromPort);
            }

            m_ConnectionDrag = {};
        }

    }

    void NodeEditor::AddNode(const std::string& typeName, ImVec2 pos)
    {
        auto nodeId = m_NodeGraph->GenerateId();
        auto node = NodeRegistry::Instance().Create(typeName, nodeId);
        if (node)
        {
            m_NodeGraph->AddNode(node);
            m_NodeViews[node->GetId()] = std::make_unique<NodeView>(node, pos);
        }
    }

    void NodeEditor::DeleteNode(Node::NodeId id)
    {
        m_NodeViews.erase(id);
        m_NodeGraph->RemoveNode(id);
    }

    void NodeEditor::StartConnectionDrag(Node::NodeId nodeId, bool output, const std::string& portName, PortType type, ImVec2 screenPos)
    {
        m_ConnectionDrag.active = true;
        m_ConnectionDrag.fromNode = nodeId;
        m_ConnectionDrag.fromOutput = output;
        m_ConnectionDrag.fromPortName = portName;
        m_ConnectionDrag.portType = type;
        m_ConnectionDrag.dragStartScreenPos = screenPos;
    }

    bool NodeEditor::IsPortCompatible(Node::NodeId nodeId, bool output, size_t portIdx, PortType type) const
    {
        if (!m_ConnectionDrag.active) return false;
        if (nodeId == m_ConnectionDrag.fromNode) return false;
        if (type != m_ConnectionDrag.portType) return false;
        if (output == m_ConnectionDrag.fromOutput) return false;
        return true;
    }
}
