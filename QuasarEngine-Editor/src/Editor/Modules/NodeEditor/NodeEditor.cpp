#include "NodeEditor.h"
#include <algorithm>
#include <iostream>

#include <glm/glm.hpp>

#include "NodeView.h"
#include "NodeEnumUtils.h"

#include <QuasarEngine/Nodes/NodeRegistry.h>

namespace QuasarEngine
{
	NodeEditor::NodeEditor(EditorContext& context) : IEditorModule(context)
    {
        for (int t = 0; t < static_cast<int>(PortType::COUNT); ++t)
        {
            PortType type = static_cast<PortType>(t);
            if (type == PortType::Unknown) continue;

            std::string display = ToString(type);
            std::string key = "Const_" + display;
            NodeRegistry::Instance().Register(
                key, display, "Constants",
                [type, display](Node::NodeId id)
                {
                    std::any defaultValue{};
                    if (type == PortType::Float) defaultValue = 0.0f;
                    else if (type == PortType::Int) defaultValue = 0;
                    else if (type == PortType::Bool) defaultValue = false;
                    else if (type == PortType::String) defaultValue = std::string{};
                    else if (type == PortType::Vec2) defaultValue = glm::vec2(0.0f);
                    else if (type == PortType::Vec3) defaultValue = glm::vec3(0.0f);
					else if (type == PortType::Vec4) defaultValue = glm::vec4(0.0f);
					else if (type == PortType::Unknown) defaultValue = {};

                    return std::make_shared<ConstNode>(id, display, type, defaultValue);
                });
        }

        for (int t = 0; t < static_cast<int>(MathOp::COUNT); ++t)
        {
            MathOp op = static_cast<MathOp>(t);
            std::string display = ToString(op);
            std::string key = "Math_" + display;
            NodeRegistry::Instance().Register(
                key, display, "Math",
                [op, display](Node::NodeId id)
                {
                    return std::make_shared<MathNode>(id, display, op);
                });
        }

        for (int t = 0; t < static_cast<int>(LogicOp::COUNT); ++t)
        {
            LogicOp op = static_cast<LogicOp>(t);
            std::string display = ToString(op);
            std::string key = "Logic_" + display;
            NodeRegistry::Instance().Register(
                key, display, "Logic",
                [op, display](Node::NodeId id)
                {
                    return std::make_shared<LogicNode>(id, display, op);
                });
        }

        for (int t = 0; t < static_cast<int>(PortType::COUNT); ++t)
        {
            PortType type = static_cast<PortType>(t);
            if (type == PortType::Unknown) continue;

            std::string display = ToString(type);
            std::string key = "Var_" + display;
            std::string label = display + " (Var)";

            NodeRegistry::Instance().Register(
                key, label, "Variables",
                [type, display](Node::NodeId id)
                {
                    auto node = std::make_shared<VariableNode>(id, display, type);

                    std::any defaultValue{};
                    if (type == PortType::Float) defaultValue = 0.0f;
                    else if (type == PortType::Int) defaultValue = 0;
                    else if (type == PortType::Bool) defaultValue = false;
                    else if (type == PortType::String) defaultValue = std::string{};
                    else if (type == PortType::Vec2) defaultValue = glm::vec2(0.0f);
                    else if (type == PortType::Vec3) defaultValue = glm::vec3(0.0f);
                    else if (type == PortType::Vec4) defaultValue = glm::vec4(0.0f);

                    node->SetValue(defaultValue);
                    return node;
                });
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
            static char searchBuf[64] = {};
            ImGui::InputTextWithHint("##SearchNodeType", "Rechercher...", searchBuf, sizeof(searchBuf));
            std::string filter = searchBuf;
            std::transform(filter.begin(), filter.end(), filter.begin(), ::tolower);

            ImGui::Separator();

            auto types = NodeRegistry::Instance().GetTypes();
            std::sort(types.begin(), types.end());

            auto starts_with = [](const std::string& s, const char* prefix)
                {
                    return s.rfind(prefix, 0) == 0;
                };

            auto pass_filter = [&](const std::string& s) -> bool
                {
                    if (filter.empty()) return true;
                    std::string low = s;
                    std::transform(low.begin(), low.end(), low.begin(), ::tolower);
                    return low.find(filter) != std::string::npos;
                };

            bool openedConst = false;
            bool openedMath = false;
            bool openedLogic = false;

            for (const auto& typeInfo : types)
            {
                if (!pass_filter(typeInfo.displayName) && !pass_filter(typeInfo.key))
                    continue;

                if (starts_with(typeInfo.key, "Const_"))
                {
                    if (!openedConst)
                    {
                        if (ImGui::TreeNodeEx("Constantes", ImGuiTreeNodeFlags_DefaultOpen))
                            openedConst = true;
                        else
                            openedConst = false;
                    }
                    if (!openedConst) continue;
                }
                else if (starts_with(typeInfo.key, "Math_"))
                {
                    if (!openedMath)
                    {
                        if (ImGui::TreeNodeEx("Math", ImGuiTreeNodeFlags_DefaultOpen))
                            openedMath = true;
                        else
                            openedMath = false;
                    }
                    if (!openedMath) continue;
                }
                else if (starts_with(typeInfo.key, "Logic_"))
                {
                    if (!openedLogic)
                    {
                        if (ImGui::TreeNodeEx("Logic", ImGuiTreeNodeFlags_DefaultOpen))
                            openedLogic = true;
                        else
                            openedLogic = false;
                    }
                    if (!openedLogic) continue;
                }

                const std::string& display = typeInfo.displayName;

                if (ImGui::MenuItem(display.c_str()))
                {
                    ImVec2 mouse = ImGui::GetIO().MousePos;
                    AddNode(typeInfo.key, m_Canvas.ScreenToCanvas(mouse));
                    ImGui::CloseCurrentPopup();
                }
            }

            if (openedConst) ImGui::TreePop();
            if (openedMath)  ImGui::TreePop();
            if (openedLogic) ImGui::TreePop();

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
        ImGui::TextColored(ImVec4(0.95f, 0.85f, 0.45f, 1.0f),
            "%s", node->GetTypeName().c_str());
        ImGui::PopFont();

        ImGui::Dummy(ImVec2(0, 6));
        ImGui::Separator();
        ImGui::Dummy(ImVec2(0, 8));

        if (auto* constNode = dynamic_cast<ConstNode*>(node.get()))
        {
            if (constNode->GetOutputPorts().empty())
            {
                ImGui::TextDisabled("Aucun port de sortie sur cette constante.");
            }
            else
            {
                PortType type = constNode->GetConstType();
                std::any valueAny = constNode->GetValue();

                if (type == PortType::Float)
                {
                    float v = 0.0f;
                    try { v = std::any_cast<float>(valueAny); }
                    catch (...) {}
                    ImGui::PushItemWidth(120);
                    if (ImGui::DragFloat("Valeur##const_float", &v, 0.1f, -10000.0f, 10000.0f, "%.3f"))
                        constNode->SetValue(v);
                    ImGui::PopItemWidth();
                }
                else if (type == PortType::Int)
                {
                    int v = 0;
                    try { v = std::any_cast<int>(valueAny); }
                    catch (...) {}
                    ImGui::PushItemWidth(120);
                    if (ImGui::InputInt("Valeur##const_int", &v))
                        constNode->SetValue(v);
                    ImGui::PopItemWidth();
                }
                else if (type == PortType::Bool)
                {
                    bool v = false;
                    try { v = std::any_cast<bool>(valueAny); }
                    catch (...) {}
                    if (ImGui::Checkbox("Valeur##const_bool", &v))
                        constNode->SetValue(v);
                }
                else if (type == PortType::String)
                {
                    static char bufConst[256] = {};
                    try
                    {
                        auto s = std::any_cast<std::string>(valueAny);
                        strncpy(bufConst, s.c_str(), sizeof(bufConst));
                        bufConst[sizeof(bufConst) - 1] = '\0';
                    }
                    catch (...) { bufConst[0] = '\0'; }

                    ImGui::PushItemWidth(180);
                    if (ImGui::InputText("Valeur##const_str", bufConst, sizeof(bufConst)))
                        constNode->SetValue(std::string(bufConst));
                    ImGui::PopItemWidth();
                }
                else if (type == PortType::Vec2)
                {
                    glm::vec2 vec(0.0f);
                    try { vec = std::any_cast<glm::vec2>(valueAny); }
                    catch (...) {}
                    float v[2] = { vec.x, vec.y };
                    ImGui::PushItemWidth(180);
                    if (ImGui::InputFloat2("Valeur##const_vec2", v))
                        constNode->SetValue(glm::vec2(v[0], v[1]));
                    ImGui::PopItemWidth();
                }
                else if (type == PortType::Vec3)
                {
                    glm::vec3 vec(0.0f);
                    try { vec = std::any_cast<glm::vec3>(valueAny); }
                    catch (...) {}
                    float v[3] = { vec.x, vec.y, vec.z };
                    ImGui::PushItemWidth(210);
                    if (ImGui::InputFloat3("Valeur##const_vec3", v))
                        constNode->SetValue(glm::vec3(v[0], v[1], v[2]));
                    ImGui::PopItemWidth();
                }
                else if (type == PortType::Vec4)
                {
                    glm::vec4 vec(0.0f);
                    try { vec = std::any_cast<glm::vec4>(valueAny); }
                    catch (...) {}
                    float v[4] = { vec.x, vec.y, vec.z, vec.w };
                    ImGui::PushItemWidth(250);
                    if (ImGui::InputFloat4("Valeur##const_vec4", v))
                        constNode->SetValue(glm::vec4(v[0], v[1], v[2], v[3]));
                    ImGui::PopItemWidth();
                }
                else
                {
                    ImGui::TextDisabled("Type non editable.");
                }
            }
        }
        else if (auto* varNode = dynamic_cast<VariableNode*>(node.get()))
        {
            const auto& outputs = varNode->GetOutputPorts();
            if (outputs.empty())
            {
                ImGui::TextDisabled("Aucun port de sortie sur cette variable.");
            }
            else
            {
                PortType type = varNode->GetVarType();
                std::any valueAny = varNode->GetValue();

                if (type == PortType::Float)
                {
                    float v = 0.0f;
                    try { v = std::any_cast<float>(valueAny); }
                    catch (...) {}
                    ImGui::PushItemWidth(120);
                    if (ImGui::DragFloat("Valeur##var_float", &v, 0.1f, -10000.0f, 10000.0f, "%.3f"))
                        varNode->SetValue(v);
                    ImGui::PopItemWidth();
                }
                else if (type == PortType::Int)
                {
                    int v = 0;
                    try { v = std::any_cast<int>(valueAny); }
                    catch (...) {}
                    ImGui::PushItemWidth(120);
                    if (ImGui::InputInt("Valeur##var_int", &v))
                        varNode->SetValue(v);
                    ImGui::PopItemWidth();
                }
                else if (type == PortType::Bool)
                {
                    bool v = false;
                    try { v = std::any_cast<bool>(valueAny); }
                    catch (...) {}
                    if (ImGui::Checkbox("Valeur##var_bool", &v))
                        varNode->SetValue(v);
                }
                else if (type == PortType::String)
                {
                    static char bufVar[256] = {};
                    try
                    {
                        auto s = std::any_cast<std::string>(valueAny);
                        strncpy(bufVar, s.c_str(), sizeof(bufVar));
                        bufVar[sizeof(bufVar) - 1] = '\0';
                    }
                    catch (...) { bufVar[0] = '\0'; }

                    ImGui::PushItemWidth(180);
                    if (ImGui::InputText("Valeur##var_str", bufVar, sizeof(bufVar)))
                        varNode->SetValue(std::string(bufVar));
                    ImGui::PopItemWidth();
                }
                else if (type == PortType::Vec2)
                {
                    glm::vec2 vec(0.0f);
                    try { vec = std::any_cast<glm::vec2>(valueAny); }
                    catch (...) {}
                    float v[2] = { vec.x, vec.y };
                    ImGui::PushItemWidth(180);
                    if (ImGui::InputFloat2("Valeur##var_vec2", v))
                        varNode->SetValue(glm::vec2(v[0], v[1]));
                    ImGui::PopItemWidth();
                }
                else if (type == PortType::Vec3)
                {
                    glm::vec3 vec(0.0f);
                    try { vec = std::any_cast<glm::vec3>(valueAny); }
                    catch (...) {}
                    float v[3] = { vec.x, vec.y, vec.z };
                    ImGui::PushItemWidth(210);
                    if (ImGui::InputFloat3("Valeur##var_vec3", v))
                        varNode->SetValue(glm::vec3(v[0], v[1], v[2]));
                    ImGui::PopItemWidth();
                }
                else if (type == PortType::Vec4)
                {
                    glm::vec4 vec(0.0f);
                    try { vec = std::any_cast<glm::vec4>(valueAny); }
                    catch (...) {}
                    float v[4] = { vec.x, vec.y, vec.z, vec.w };
                    ImGui::PushItemWidth(250);
                    if (ImGui::InputFloat4("Valeur##var_vec4", v))
                        varNode->SetValue(glm::vec4(v[0], v[1], v[2], v[3]));
                    ImGui::PopItemWidth();
                }
                else
                {
                    ImGui::TextDisabled("Type non editable.");
                }
            }
        }
        else if (auto* mathNode = dynamic_cast<MathNode*>(node.get()))
        {
            ImGui::TextDisabled("Noeud Math");
            ImGui::Dummy(ImVec2(0, 4));

            int op = static_cast<int>(mathNode->GetOperation());
            const char* labels[] = { "Add", "Sub", "Mul", "Div" };
            if (ImGui::Combo("Operation", &op, labels, IM_ARRAYSIZE(labels)))
            {
                mathNode->SetOperation(static_cast<MathOp>(op));
            }
        }
        else if (auto* logicNode = dynamic_cast<LogicNode*>(node.get()))
        {
            ImGui::TextDisabled("Noeud Logic");
            ImGui::Dummy(ImVec2(0, 4));

            int op = static_cast<int>(logicNode->GetOperation());
            const char* labels[] = { "And", "Or", "Not", "Xor" };
            if (ImGui::Combo("Operation", &op, labels, IM_ARRAYSIZE(labels)))
            {
                logicNode->SetOperation(static_cast<LogicOp>(op));
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
            DrawConnectionCurve(fromPos, toPos, false, nullptr);
        }

        for (const auto& conn : m_NodeGraph->GetConnections())
        {
            auto fromNode = conn->fromNode.lock();
            auto toNode = conn->toNode.lock();
            if (!fromNode || !toNode) continue;

            auto* fromView = GetNodeView(fromNode->GetId());
            auto* toView = GetNodeView(toNode->GetId());
            if (!fromView || !toView) continue;

            ImVec2 fromPos = fromView->GetPortScreenPos(conn->fromPort, true,
                m_Canvas.pan, m_Canvas.zoom, m_Canvas.canvasPos);
            ImVec2 toPos = toView->GetPortScreenPos(conn->toPort, false,
                m_Canvas.pan, m_Canvas.zoom, m_Canvas.canvasPos);

            DrawConnectionCurve(fromPos, toPos, false, conn.get());
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

    void NodeEditor::DrawConnectionCurve(const ImVec2& from, const ImVec2& to,
        bool selected, NodeConnection* connection)
    {
        ImDrawList* drawList = ImGui::GetWindowDrawList();

        float dist = fabsf(to.x - from.x) * 0.5f;
        ImVec2 cp1(from.x + dist, from.y);
        ImVec2 cp2(to.x - dist, to.y);

        bool hovered = false;
        if (connection)
        {
            ImVec2 mouse = ImGui::GetIO().MousePos;

            const int segments = 20;
            float minDist2 = FLT_MAX;
            for (int i = 0; i <= segments; ++i)
            {
                float t = static_cast<float>(i) / static_cast<float>(segments);
                float u = 1.0f - t;

                ImVec2 p;
                p.x = u * u * u * from.x +
                    3 * u * u * t * cp1.x +
                    3 * u * t * t * cp2.x +
                    t * t * t * to.x;
                p.y = u * u * u * from.y +
                    3 * u * u * t * cp1.y +
                    3 * u * t * t * cp2.y +
                    t * t * t * to.y;

                float dx = p.x - mouse.x;
                float dy = p.y - mouse.y;
                float d2 = dx * dx + dy * dy;
                if (d2 < minDist2) minDist2 = d2;
            }

            const float maxHoverDist = 6.0f;
            hovered = (minDist2 <= maxHoverDist * maxHoverDist);

            if (hovered && ImGui::IsMouseClicked(ImGuiMouseButton_Right))
            {
                auto fromNode = connection->fromNode.lock();
                auto toNode = connection->toNode.lock();
                if (fromNode && toNode)
                {
                    m_NodeGraph->Disconnect(fromNode->GetId(), connection->fromPort,
                        toNode->GetId(), connection->toPort);
                }
                return;
            }
        }

        ImU32 color = selected || hovered
            ? IM_COL32(255, 200, 0, 255)
            : IM_COL32(200, 200, 200, 200);

        float thickness = selected || hovered ? 3.0f : 2.0f;

        drawList->AddBezierCubic(from, cp1, cp2, to, color, thickness);
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

    void NodeEditor::AddNode(const std::string& typeKey, ImVec2 pos)
    {
        auto nodeId = m_NodeGraph->GenerateId();
        auto node = NodeRegistry::Instance().Create(typeKey, nodeId);
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
