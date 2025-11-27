#include "NodeEditor.h"

#include <algorithm>
#include <iostream>

#include <glm/glm.hpp>

#include "NodeView.h"

#include <QuasarEngine/Nodes/NodeEnumUtils.h>
#include <QuasarEngine/Nodes/NodeRegistry.h>
#include <QuasarEngine/Nodes/ShaderGraphCompiler.h>

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

        NodeRegistry::Instance().Register(
            "Vec2_Components", "Vec2 (X,Y)", "Math/Vector",
            [](Node::NodeId id)
            {
                return std::make_shared<Vec2ComponentsNode>(id, "Vec2 Components");
            });

        NodeRegistry::Instance().Register(
            "Vec3_Components", "Vec3 (X,Y,Z)", "Math/Vector",
            [](Node::NodeId id)
            {
                return std::make_shared<Vec3ComponentsNode>(id, "Vec3 Components");
            });

        NodeRegistry::Instance().Register(
            "Vec4_Components", "Vec4 (X,Y,Z,W)", "Math/Vector",
            [](Node::NodeId id)
            {
                return std::make_shared<Vec4ComponentsNode>(id, "Vec4 Components");
            });

        NodeRegistry::Instance().Register(
            "Texture_Sample", "Texture Sample", "Textures",
            [](Node::NodeId id)
            {
                return std::make_shared<TextureSampleNode>(id, "Texture Sample");
            });

        NodeRegistry::Instance().Register(
            "Math_ClampFloat", "Clamp (Float)", "Math/Scalar",
            [](Node::NodeId id)
            {
                return std::make_shared<ClampFloatNode>(id, "Clamp Float");
            });

        NodeRegistry::Instance().Register(
            "Math_ClampVec3", "Clamp (Vec3)", "Math/Vector",
            [](Node::NodeId id)
            {
                return std::make_shared<ClampVec3Node>(id, "Clamp Vec3");
            });

        NodeRegistry::Instance().Register(
            "Math_LerpFloat", "Lerp (Float)", "Math/Scalar",
            [](Node::NodeId id)
            {
                return std::make_shared<LerpFloatNode>(id, "Lerp Float");
            });

        NodeRegistry::Instance().Register(
            "Math_LerpVec3", "Lerp (Vec3)", "Math/Vector",
            [](Node::NodeId id)
            {
                return std::make_shared<LerpVec3Node>(id, "Lerp Vec3");
            });

        NodeRegistry::Instance().Register(
            "Math_LengthVec2", "Length (Vec2)", "Math/Vector",
            [](Node::NodeId id)
            {
                return std::make_shared<LengthVec2Node>(id, "Length Vec2");
            });

        NodeRegistry::Instance().Register(
            "Math_LengthVec3", "Length (Vec3)", "Math/Vector",
            [](Node::NodeId id)
            {
                return std::make_shared<LengthVec3Node>(id, "Length Vec3");
            });

        NodeRegistry::Instance().Register(
            "Math_LengthVec4", "Length (Vec4)", "Math/Vector",
            [](Node::NodeId id)
            {
                return std::make_shared<LengthVec4Node>(id, "Length Vec4");
            });

        NodeRegistry::Instance().Register(
            "Math_NormalizeVec2", "Normalize (Vec2)", "Math/Vector",
            [](Node::NodeId id)
            {
                return std::make_shared<NormalizeVec2Node>(id, "Normalize Vec2");
            });

        NodeRegistry::Instance().Register(
            "Math_NormalizeVec3", "Normalize (Vec3)", "Math/Vector",
            [](Node::NodeId id)
            {
                return std::make_shared<NormalizeVec3Node>(id, "Normalize Vec3");
            });

        NodeRegistry::Instance().Register(
            "Math_NormalizeVec4", "Normalize (Vec4)", "Math/Vector",
            [](Node::NodeId id)
            {
                return std::make_shared<NormalizeVec4Node>(id, "Normalize Vec4");
            });

        NodeRegistry::Instance().Register(
            "Math_DotVec2", "Dot (Vec2)", "Math/Vector",
            [](Node::NodeId id)
            {
                return std::make_shared<DotVec2Node>(id, "Dot Vec2");
            });

        NodeRegistry::Instance().Register(
            "Math_DotVec3", "Dot (Vec3)", "Math/Vector",
            [](Node::NodeId id)
            {
                return std::make_shared<DotVec3Node>(id, "Dot Vec3");
            });

        NodeRegistry::Instance().Register(
            "Math_DotVec4", "Dot (Vec4)", "Math/Vector",
            [](Node::NodeId id)
            {
                return std::make_shared<DotVec4Node>(id, "Dot Vec4");
            });

        NodeRegistry::Instance().Register(
            "Math_CrossVec3", "Cross (Vec3)", "Math/Vector",
            [](Node::NodeId id)
            {
                return std::make_shared<CrossVec3Node>(id, "Cross Vec3");
            });

        NodeRegistry::Instance().Register(
            "Utils_Time", "Time", "Utils",
            [](Node::NodeId id)
            {
                return std::make_shared<TimeNode>(id, "Time");
            });

        NodeRegistry::Instance().Register(
            "Utils_RandomFloat", "Random (Float)", "Utils",
            [](Node::NodeId id)
            {
                return std::make_shared<RandomFloatNode>(id, "Random Float");
            });

        NodeRegistry::Instance().Register(
            "Utils_CompareFloat", "Compare (Float)", "Utils",
            [](Node::NodeId id)
            {
                return std::make_shared<CompareFloatNode>(id, "Compare Float");
            });

        NodeRegistry::Instance().Register(
            "Tex_Coords", "Texture Coords", "Textures/Coords",
            [](Node::NodeId id)
            {
                return std::make_shared<TextureCoordinateNode>(id, "Texture Coords");
            });

        NodeRegistry::Instance().Register(
            "Tex_Color", "Color", "Textures/Color",
            [](Node::NodeId id)
            {
                return std::make_shared<ColorNode>(id, "Color");
            });

        NodeRegistry::Instance().Register(
            "Tex_ColorMask", "Color Mask", "Textures/Color",
            [](Node::NodeId id)
            {
                return std::make_shared<ColorMaskNode>(id, "Color Mask");
            });

        NodeRegistry::Instance().Register(
            "Output_Float", "Output Float", "Output/Debug",
            [](Node::NodeId id)
            {
                return std::make_shared<FloatOutputNode>(id, "Output Float");
            });

        NodeRegistry::Instance().Register(
            "Output_Vec3", "Output Vec3", "Output/Debug",
            [](Node::NodeId id)
            {
                return std::make_shared<Vec3OutputNode>(id, "Output Vec3");
            });

        NodeRegistry::Instance().Register(
            "Output_Material", "Material Output", "Output/Material",
            [](Node::NodeId id)
            {
                return std::make_shared<MaterialOutputNode>(id, "Material Output");
            });

        m_NodeGraph = std::make_shared<NodeGraph>();
    }

    NodeEditor::~NodeEditor() {}

    void NodeEditor::Update(double dt)
    {
        if (m_GraphDirty && m_NodeGraph)
        {
            m_NodeGraph->Evaluate();
            m_GraphDirty = false;
        }
    }

    void NodeEditor::Render()
    {

    }

    void NodeEditor::RenderUI()
    {
        ImGui::Begin("Node Editor", nullptr);

        ImVec2 fullSize = ImGui::GetContentRegionAvail();
        if (fullSize.x <= 0 || fullSize.y <= 0)
        {
            ImGui::End();
            return;
        }

        float canvasWidth = fullSize.x * 0.7f;

        ImGui::BeginChild("NodeCanvasRegion",
            ImVec2(canvasWidth, 0),
            false,
            ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse);

        ImVec2 canvasPos = ImGui::GetCursorScreenPos();
        ImVec2 canvasSize = ImGui::GetContentRegionAvail();
        m_Canvas.BeginRegion(canvasPos, canvasSize);
        m_Canvas.baseGridStep = 40.0f;

        ImGui::SetCursorScreenPos(m_Canvas.canvasPos + ImVec2(10, 10));
        ImGui::SetItemAllowOverlap();
        bool wantOpenPopup = false;
        if (ImGui::Button("+ Ajouter un noeud", ImVec2(150, 0)))
            wantOpenPopup = true;

        if (wantOpenPopup)
            ImGui::OpenPopup("AddNodePopup");

        bool hoveringCanvas = ImGui::IsWindowHovered(ImGuiHoveredFlags_AllowWhenBlockedByActiveItem);
        bool activeCanvas = ImGui::IsWindowFocused(ImGuiFocusedFlags_ChildWindows);

        ShowGraph();
        HandleEvents(hoveringCanvas, activeCanvas);

        if (hoveringCanvas && ImGui::IsMouseClicked(ImGuiMouseButton_Right))
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

            auto pass_filter = [&](const std::string& s) -> bool
                {
                    if (filter.empty()) return true;
                    std::string low = s;
                    std::transform(low.begin(), low.end(), low.begin(), ::tolower);
                    return low.find(filter) != std::string::npos;
                };

            std::string currentCategory;
            bool categoryOpen = true;

            for (const auto& typeInfo : types)
            {
                if (!pass_filter(typeInfo.displayName) && !pass_filter(typeInfo.key))
                    continue;

                if (typeInfo.category != currentCategory)
                {
                    currentCategory = typeInfo.category;

                    categoryOpen = ImGui::CollapsingHeader(
                        currentCategory.c_str(),
                        ImGuiTreeNodeFlags_DefaultOpen
                    );
                }

                if (!categoryOpen)
                    continue;

                ImGui::Indent(12.0f);
                if (ImGui::Selectable(typeInfo.displayName.c_str()))
                {
                    ImVec2 mouse = ImGui::GetIO().MousePos;
                    AddNode(typeInfo.key, m_Canvas.ScreenToCanvas(mouse));
                    ImGui::CloseCurrentPopup();
                }
                ImGui::Unindent(12.0f);
            }

            ImGui::EndPopup();
        }

        ImGui::EndChild();

        ImGui::SameLine();

        ImGui::BeginChild("InspectorRegion", ImVec2(0, 0), true);
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
            PortType type = constNode->GetConstType();
            std::any defAny = constNode->GetDefaultValue();

            if (type == PortType::Float)
            {
                float v = 0.0f;
                try { v = std::any_cast<float>(defAny); }
                catch (...) {}
                ImGui::PushItemWidth(120);
                if (ImGui::DragFloat("Valeur", &v, 0.1f, -10000.0f, 10000.0f, "%.3f"))
                {
                    constNode->SetDefaultValue(v);
                    MarkGraphDirty();
                }
                ImGui::PopItemWidth();
            }
            else if (type == PortType::Int)
            {
                int v = 0;
                try { v = std::any_cast<int>(defAny); }
                catch (...) {}
                ImGui::PushItemWidth(120);
                if (ImGui::InputInt("Valeur", &v))
                {
                    constNode->SetDefaultValue(v);
                    MarkGraphDirty();
                }
                ImGui::PopItemWidth();
            }
            else if (type == PortType::Bool)
            {
                bool v = false;
                try { v = std::any_cast<bool>(defAny); }
                catch (...) {}
                if (ImGui::Checkbox("Valeur", &v))
                {
                    constNode->SetDefaultValue(v);
                    MarkGraphDirty();
                }
            }
            else if (type == PortType::String)
            {
                static char buf[256] = {};
                try
                {
                    auto s = std::any_cast<std::string>(defAny);
                    strncpy(buf, s.c_str(), sizeof(buf));
                    buf[sizeof(buf) - 1] = '\0';
                }
                catch (...) { buf[0] = '\0'; }

                ImGui::PushItemWidth(180);
                if (ImGui::InputText("Valeur", buf, sizeof(buf)))
                {
                    constNode->SetDefaultValue(std::string(buf));
                    MarkGraphDirty();
                }
                ImGui::PopItemWidth();
            }
            else if (type == PortType::Vec2)
            {
                glm::vec2 vec(0.0f);
                try { vec = std::any_cast<glm::vec2>(defAny); }
                catch (...) {}
                float v[2] = { vec.x, vec.y };
                ImGui::PushItemWidth(210);
                if (ImGui::InputFloat2("Valeur", v))
                {
                    constNode->SetDefaultValue(glm::vec2(v[0], v[1]));
                    MarkGraphDirty();
                }
                ImGui::PopItemWidth();
            }
            else if (type == PortType::Vec3)
            {
                glm::vec3 vec(0.0f);
                try { vec = std::any_cast<glm::vec3>(defAny); }
                catch (...) {}
                float v[3] = { vec.x, vec.y, vec.z };
                ImGui::PushItemWidth(210);
                if (ImGui::InputFloat3("Valeur", v))
                {
                    constNode->SetDefaultValue(glm::vec3(v[0], v[1], v[2]));
                    MarkGraphDirty();
                }
                ImGui::PopItemWidth();
            }
            else if (type == PortType::Vec4)
            {
                glm::vec4 vec(0.0f);
                try { vec = std::any_cast<glm::vec4>(defAny); }
                catch (...) {}
                float v[4] = { vec.x, vec.y, vec.z, vec.w };
                ImGui::PushItemWidth(210);
                if (ImGui::InputFloat4("Valeur", v))
                {
                    constNode->SetDefaultValue(glm::vec4(v[0], v[1], v[2], v[3]));
                    MarkGraphDirty();
                }
                ImGui::PopItemWidth();
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
        else if (auto* texNode = dynamic_cast<TextureSampleNode*>(node.get()))
        {
            ImGui::TextDisabled("Texture");

            char buf[260] = {};
            {
                auto path = texNode->GetRelativePath();
                strncpy(buf, path.c_str(), sizeof(buf));
                buf[sizeof(buf) - 1] = '\0';
            }

            ImGui::PushItemWidth(-1.0f);
            ImGui::TextDisabled("Chemin (Assets/..)");
            if (ImGui::InputText("##path", buf, sizeof(buf)))
            {
                texNode->SetRelativePath(std::string(buf));
				std::cout << "Texture path set to: " << buf << std::endl;
                texNode->SetImGuiTextureId(nullptr);
            }
            ImGui::PopItemWidth();

            ImGui::Dummy(ImVec2(0, 6));

            const std::string& relPath = texNode->GetRelativePath();
            if (!relPath.empty())
            {
                if (!AssetManager::Instance().isAssetLoaded(relPath))
                {
                    AssetToLoad asset;
                    asset.path = AssetManager::Instance().ResolvePath(relPath).generic_string();
                    asset.id = relPath;
                    asset.type = AssetType::TEXTURE;
                    asset.spec = TextureSpecification{};

                    std::cout << "Loading texture asset: " << asset.path
                        << " with id: " << asset.id << std::endl;

                    AssetManager::Instance().loadAsset(asset);
                }

                if (AssetManager::Instance().isAssetLoaded(relPath))
                {
                    std::shared_ptr<Texture2D> tex =
                        AssetManager::Instance().getAsset<Texture2D>(relPath);
                    texNode->SetImGuiTextureId((void*)tex->GetHandle());
                    MarkGraphDirty();
                }
            }

            if (void* texId = texNode->GetImGuiTextureId())
            {
                ImVec2 size(96, 96);
                ImGui::Image((ImTextureID)texId, size);
            }
            else
            {
                ImGui::TextDisabled("Aucune texture chargee.");
            }
        }
        else if (auto* colorNode = dynamic_cast<ColorNode*>(node.get()))
        {
            ImGui::TextDisabled("Couleur constante");

            glm::vec4 col = colorNode->GetColor();
            float c[4] = { col.r, col.g, col.b, col.a };
            if (ImGui::ColorEdit4("Color", c))
            {
                colorNode->SetColor(glm::vec4(c[0], c[1], c[2], c[3]));
                MarkGraphDirty();
            }
        }
        else if (auto* cmpNode = dynamic_cast<CompareFloatNode*>(node.get()))
        {
            ImGui::TextDisabled("Comparaison de floats");

            int op = static_cast<int>(cmpNode->GetOperation());
            const char* labels[] = {
                "A < B",
                "A <= B",
                "A > B",
                "A >= B",
                "A == B",
                "A != B"
            };
            if (ImGui::Combo("Operation", &op, labels, IM_ARRAYSIZE(labels)))
            {
                cmpNode->SetOperation(static_cast<CompareOp>(op));
            }
        }
        else if (auto* maskNode = dynamic_cast<ColorMaskNode*>(node.get()))
        {
            ImGui::TextDisabled("Color Mask (masquage RGBA)");

            bool useR = maskNode->GetUseR();
            bool useG = maskNode->GetUseG();
            bool useB = maskNode->GetUseB();
            bool useA = maskNode->GetUseA();

            if (ImGui::Checkbox("Use R", &useR)) { maskNode->SetUseR(useR); MarkGraphDirty(); }
            if (ImGui::Checkbox("Use G", &useG)) { maskNode->SetUseG(useG); MarkGraphDirty(); }
            if (ImGui::Checkbox("Use B", &useB)) { maskNode->SetUseB(useB); MarkGraphDirty(); }
            if (ImGui::Checkbox("Use A", &useA)) { maskNode->SetUseA(useA); MarkGraphDirty(); }
        }
        else if (auto* floatOut = dynamic_cast<FloatOutputNode*>(node.get()))
        {
            ImGui::TextDisabled("Nœud de sortie (Float)");

            float v = floatOut->GetInput<float>("Value", 0.0f);
            ImGui::Dummy(ImVec2(0, 4));
            ImGui::Text("Valeur actuelle : %.3f", v);
        }
        else if (auto* vec3Out = dynamic_cast<Vec3OutputNode*>(node.get()))
        {
            ImGui::TextDisabled("Nœud de sortie (Vec3)");

            glm::vec3 v = vec3Out->GetInput<glm::vec3>("Value", glm::vec3(0.0f));
            ImGui::Dummy(ImVec2(0, 4));
            ImGui::Text("X : %.3f", v.x);
            ImGui::Text("Y : %.3f", v.y);
            ImGui::Text("Z : %.3f", v.z);
        }
        else if (auto* matOut = dynamic_cast<MaterialOutputNode*>(node.get()))
        {
            ImGui::TextDisabled("Material Output (PBR)");

            glm::vec3 baseColor = matOut->GetBaseColor();
            float metallic = matOut->GetMetallic();
            float roughness = matOut->GetRoughness();
            glm::vec3 emissive = matOut->GetEmissive();
            float opacity = matOut->GetOpacity();

            ImGui::Dummy(ImVec2(0, 4));
            ImGui::Text("BaseColor : (%.3f, %.3f, %.3f)",
                baseColor.r, baseColor.g, baseColor.b);
            ImGui::Text("Metallic  : %.3f", metallic);
            ImGui::Text("Roughness : %.3f", roughness);
            ImGui::Text("Emissive  : (%.3f, %.3f, %.3f)",
                emissive.r, emissive.g, emissive.b);
            ImGui::Text("Opacity   : %.3f", opacity);

            ImGui::Separator();

            static std::string g_LastShaderCode;
            if (ImGui::Button("Générer shader"))
            {
				Node::NodeId matOutNodeId = matOut->GetId();

                ShaderGraphCompileOptions option;
				option.shaderName = "my_material_01";

                auto result = ShaderGraphCompiler::CompileMaterialPBR(
                    *m_NodeGraph,
                    matOutNodeId,
                    option
                );

                g_LastShaderCode = result.fragmentSource;
            }

            ImGui::TextUnformatted(g_LastShaderCode.c_str());
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
                    m_NodeGraph->Disconnect(fromNode->GetId(), connection->fromPort, toNode->GetId(), connection->toPort);
                    MarkGraphDirty();
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
                {
                    m_NodeGraph->Connect(fromNode, fromPort, toNode, toPort);
                    MarkGraphDirty();
                }
                else if (!m_ConnectionDrag.fromOutput && bestIsOutput)
                {
                    m_NodeGraph->Connect(toNode, toPort, fromNode, fromPort);
                    MarkGraphDirty();
                }
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
            MarkGraphDirty();
        }
    }

    void NodeEditor::DeleteNode(Node::NodeId id)
    {
        m_NodeViews.erase(id);
        m_NodeGraph->RemoveNode(id);
        MarkGraphDirty();
    }

    bool NodeEditor::IsInputConnected(Node::NodeId nodeId, const std::string& portName) const
    {
        if (!m_NodeGraph)
            return false;

        auto conn = m_NodeGraph->FindConnectionTo(nodeId, portName);
        return conn != nullptr;
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
