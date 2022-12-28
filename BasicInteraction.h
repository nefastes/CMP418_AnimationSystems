#pragma once
# include "imgui.h"
# include "node-editor\imgui_node_editor.h"
# include "node-editor\application.h"
#include "Animation3D.h"

namespace ed = ax::NodeEditor;

struct BasicInteractionExample :
    public Application
{
    // Struct to hold basic information about connection between
    // pins. Note that connection (aka. link) has its own ID.
    // This is useful later with dealing with selections, deletion
    // or other operations.
    struct LinkInfo
    {
        ed::LinkId Id;
        ed::PinId  InputId;
        ed::PinId  OutputId;
    };

    using Application::Application;

    void OnStart() override
    {
        ed::Config config;
        config.SettingsFile = "BasicInteraction.json";
        m_Context = ed::CreateEditor(&config);
    }

    void OnStop() override
    {
        ed::DestroyEditor(m_Context);
    }

    void ImGuiEx_BeginColumn()
    {
        ImGui::BeginGroup();
    }

    void ImGuiEx_NextColumn()
    {
        ImGui::EndGroup();
        ImGui::SameLine();
        ImGui::BeginGroup();
    }

    void ImGuiEx_EndColumn()
    {
        ImGui::EndGroup();
    }

    void OnFrame(float deltaTime) override
    {
        auto& io = ImGui::GetIO();

        ImGui::Text("FPS: %.2f (%.2gms)", io.Framerate, io.Framerate ? 1000.0f / io.Framerate : 0.0f);

        ImGui::Separator();

        ed::SetCurrentEditor(m_Context);

        // Start interaction with editor.
        ed::Begin("My Editor", ImVec2(0.0, 0.0f));

        int uniqueId = 1;

        //
        // 1) Commit known data to editor
        //

        const std::vector<BlendNode*>& treeToDraw = p_SentAnim->GetBlendTree()->GetTree();
        for (size_t i = 0u; i < treeToDraw.size(); ++i)
        {
            ImGui::PushID(i);
            // Draw a UI node for each blendtree node
            // Use different patterns based on their types
            ed::NodeId node_Id = uniqueId++;
            ed::PinId  node_InputPinId = uniqueId++;
            ed::PinId  node_OutputPinId = uniqueId++;

            BlendNode* node = treeToDraw[i];
            switch (node->GetType())
            {
            case NodeType_::NodeType_Output:
                ed::BeginNode(node_Id);
                ImGui::Text("Output Node");
                ed::BeginPin(node_InputPinId, ed::PinKind::Input);
                ImGui::Text("-> In");
                ed::EndPin();
                ed::EndNode();
                break;
            case NodeType_::NodeType_Clip:
                {
                    ed::BeginNode(node_Id);
                    ImGui::Text("Clip Node");
                    ImGuiEx_BeginColumn();
                    ImGui::PushItemWidth(200);

                    const std::vector<std::string>& availableAnims = p_SentAnim->AvailableClips();
                    ClipNode* clipNode = reinterpret_cast<ClipNode*>(node);
                    const char* clipName = clipNode->GetClip()->name.c_str();

                    ImGui::Text("Clip:");
                    ImGui::SameLine();
                    if (ImGui::Button(clipName))
                        ImGui::OpenPopup("clip");

                    
                    ImGui::PopItemWidth();
                    ImGuiEx_NextColumn();
                    ed::BeginPin(node_OutputPinId, ed::PinKind::Output);
                    ImGui::Text("Out ->");
                    ed::EndPin();
                    ImGuiEx_EndColumn();
                    ed::EndNode();

                    // This is the actual popup Gui drawing section.
                    ed::Suspend();
                    if (ImGui::BeginPopup("clip")) {
                        // Note: if it weren't for the child window, we would have to PushItemWidth() here to avoid a crash!
                        ImGui::TextDisabled("Pick One:");
                        ImGui::BeginChild("popup_scroller", ImVec2(200, 100), true, ImGuiWindowFlags_AlwaysVerticalScrollbar);
                        for (size_t j = 0u; j < availableAnims.size(); ++j)
                        {
                            if (ImGui::Button(availableAnims[j].c_str())) {
                                clipNode->SetClip(p_SentAnim->GetClip(j));
                                ImGui::CloseCurrentPopup();  // These calls revoke the popup open state, which was set by OpenPopup above.
                            }
                        }
                        ImGui::EndChild();
                        ImGui::EndPopup(); // Note this does not do anything to the popup open/close state. It just terminates the content declaration.
                    }
                    ed::Resume();
                }
                break;
            case NodeType_::NodeType_LinearBlend:
                {
                    ed::PinId  node_InputPinId2 = uniqueId++;
                    ed::BeginNode(node_Id);
                    ImGui::Text("Linear Blend");
                    ImGuiEx_BeginColumn();
                    ed::BeginPin(node_InputPinId, ed::PinKind::Input);
                    ImGui::Text("-> In1");
                    ed::EndPin();
                    ed::BeginPin(node_InputPinId2, ed::PinKind::Input);
                    ImGui::Text("-> In2");
                    ed::EndPin();

                    LinearBlendNode* blendNode = reinterpret_cast<LinearBlendNode*>(node);
                    ImGui::PushItemWidth(200);
                    ImGui::SliderFloat("Blend Factor", blendNode->GetBlendValue(), 0.f, 1.f);
                    ImGui::PopItemWidth();

                    ImGuiEx_NextColumn();
                    ed::BeginPin(node_OutputPinId, ed::PinKind::Output);
                    ImGui::Text("Out ->");
                    ed::EndPin();
                    ImGuiEx_EndColumn();
                    ed::EndNode();
                }
                break;
            case NodeType_::NodeType_LinearBlendSync:
                {
                    ed::PinId  node_InputPinId2 = uniqueId++;
                    ed::BeginNode(node_Id);
                    ImGui::Text("Linear Blend Sync");
                    ImGuiEx_BeginColumn();
                    ed::BeginPin(node_InputPinId, ed::PinKind::Input);
                    ImGui::Text("-> In1");
                    ed::EndPin();
                    ed::BeginPin(node_InputPinId2, ed::PinKind::Input);
                    ImGui::Text("-> In2");
                    ed::EndPin();

                    LinearBlendNodeSync* blendNode = reinterpret_cast<LinearBlendNodeSync*>(node);
                    ImGui::PushItemWidth(200);
                    ImGui::SliderFloat("Blend Factor", blendNode->GetBlendValue(), 0.f, 1.f);
                    ImGui::PopItemWidth();

                    ImGuiEx_NextColumn();
                    ed::BeginPin(node_OutputPinId, ed::PinKind::Output);
                    ImGui::Text("Out ->");
                    ed::EndPin();
                    ImGuiEx_EndColumn();
                    ed::EndNode();
                }
                break;
            default:
                break;
            }
            ImGui::PopID();
        }

        //// Submit Node A
        //ed::NodeId nodeA_Id = uniqueId++;
        //ed::PinId  nodeA_InputPinId = uniqueId++;
        //ed::PinId  nodeA_OutputPinId = uniqueId++;

        //if (m_FirstFrame)
        //    ed::SetNodePosition(nodeA_Id, ImVec2(10, 10));
        //ed::BeginNode(nodeA_Id);
        //ImGui::Text("Node A");
        //ed::BeginPin(nodeA_InputPinId, ed::PinKind::Input);
        //ImGui::Text("-> In");
        //ed::EndPin();
        //ImGui::SameLine();
        //ed::BeginPin(nodeA_OutputPinId, ed::PinKind::Output);
        //ImGui::Text("Out ->");
        //ed::EndPin();
        //ed::EndNode();

        //// Submit Node B
        //ed::NodeId nodeB_Id = uniqueId++;
        //ed::PinId  nodeB_InputPinId1 = uniqueId++;
        //ed::PinId  nodeB_InputPinId2 = uniqueId++;
        //ed::PinId  nodeB_OutputPinId = uniqueId++;

        //if (m_FirstFrame)
        //    ed::SetNodePosition(nodeB_Id, ImVec2(210, 60));
        //ed::BeginNode(nodeB_Id);
        //ImGui::Text("Node B");
        //ImGuiEx_BeginColumn();
        //ed::BeginPin(nodeB_InputPinId1, ed::PinKind::Input);
        //ImGui::Text("-> In1");
        //ed::EndPin();
        //ed::BeginPin(nodeB_InputPinId2, ed::PinKind::Input);
        //ImGui::Text("-> In2");
        //ed::EndPin();
        //ImGuiEx_NextColumn();
        //ed::BeginPin(nodeB_OutputPinId, ed::PinKind::Output);
        //ImGui::Text("Out ->");
        //ed::EndPin();
        //ImGuiEx_EndColumn();
        //ed::EndNode();

        // Submit Links
        for (auto& linkInfo : m_Links)
            ed::Link(linkInfo.Id, linkInfo.InputId, linkInfo.OutputId);

        //
        // 2) Handle interactions
        //

        // Handle creation action, returns true if editor want to create new object (node or link)
        if (ed::BeginCreate())
        {
            ed::PinId inputPinId, outputPinId;
            if (ed::QueryNewLink(&inputPinId, &outputPinId))
            {
                // QueryNewLink returns true if editor want to create new link between pins.
                //
                // Link can be created only for two valid pins, it is up to you to
                // validate if connection make sense. Editor is happy to make any.
                //
                // Link always goes from input to output. User may choose to drag
                // link from output pin or input pin. This determine which pin ids
                // are valid and which are not:
                //   * input valid, output invalid - user started to drag new ling from input pin
                //   * input invalid, output valid - user started to drag new ling from output pin
                //   * input valid, output valid   - user dragged link over other pin, can be validated

                if (inputPinId && outputPinId) // both are valid, let's accept link
                {
                    // ed::AcceptNewItem() return true when user release mouse button.
                    if (ed::AcceptNewItem())
                    {
                        // Since we accepted new link, lets add one to our list of links.
                        m_Links.push_back({ ed::LinkId(m_NextLinkId++), inputPinId, outputPinId });

                        // Draw new link.
                        ed::Link(m_Links.back().Id, m_Links.back().InputId, m_Links.back().OutputId);
                    }

                    // You may choose to reject connection between these nodes
                    // by calling ed::RejectNewItem(). This will allow editor to give
                    // visual feedback by changing link thickness and color.
                }
            }
        }
        ed::EndCreate(); // Wraps up object creation action handling.


        // Handle deletion action
        if (ed::BeginDelete())
        {
            // There may be many links marked for deletion, let's loop over them.
            ed::LinkId deletedLinkId;
            while (ed::QueryDeletedLink(&deletedLinkId))
            {
                // If you agree that link can be deleted, accept deletion.
                if (ed::AcceptDeletedItem())
                {
                    // Then remove link from your data.
                    for (auto& link : m_Links)
                    {
                        if (link.Id == deletedLinkId)
                        {
                            m_Links.erase(&link);
                            break;
                        }
                    }
                }

                // You may reject link deletion by calling:
                // ed::RejectDeletedItem();
            }
        }
        ed::EndDelete(); // Wrap up deletion action



        // End of interaction with editor.
        ed::End();

        if (m_FirstFrame)
            ed::NavigateToContent(0.0f);

        ed::SetCurrentEditor(nullptr);

        m_FirstFrame = false;

        // ImGui::ShowMetricsWindow();
    }

    ed::EditorContext*      m_Context = nullptr;    // Editor context, required to trace a editor state.
    bool                    m_FirstFrame = true;    // Flag set for first frame only, some action need to be executed once.
    ImVector<LinkInfo>      m_Links;                // List of live links. It is dynamic unless you want to create read-only view over nodes.
    int                     m_NextLinkId = 100;     // Counter to help generate link ids. In real application this will probably based on pointer to user data structure.
    AsdfAnim::Animation3D*  p_SentAnim = nullptr;
};