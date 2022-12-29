#pragma once
# include "imgui.h"
# include "node-editor\imgui_node_editor.h"
# include "node-editor\application.h"
#include "Animation3D.h"
#include <unordered_map>
#include <functional>
#include <array>

namespace ed = ax::NodeEditor;

struct BasicInteractionExample :
    public Application
{
    struct UINode
    {
        BlendNode* animationNode;
        ed::NodeId nodeID;
        std::array<ed::PinId, 4> inputPinIDs;
        std::array<bool, 4> usedInputPinIDs;
        ed::PinId outputPinID;
        std::function<void(UINode* const thisPtr, AsdfAnim::Animation3D* sentAnim)> Draw;
    };

    UINode* FindUINodeFromAnimationNode(BlendNode* const& itemToSearch)
    {
        // TERRIBLE way of doing this
        // but since this will be used for relatively small lookups (4 - 5 items in v_Nodes and 4 calls max)
        // it will do for now
        for (UINode& uiNode : v_Nodes)
            if (uiNode.animationNode == itemToSearch)
                return &uiNode;
        return nullptr;
    }

    UINode* FindUINodeFromOutputPinID(const ed::PinId& itemToSearch)
    {
        // TERRIBLE way of doing this
        // but since this will be used for relatively small lookups (4 - 5 items in v_Nodes and 4 calls max)
        // it will do for now
        for (UINode& uiNode : v_Nodes)
            if (uiNode.outputPinID == itemToSearch)
                return &uiNode;
        return nullptr;
    }

    UINode* FindUINodeFromInputPinID(const ed::PinId& itemToSearch)
    {
        // TERRIBLE way of doing this
        // but since this will be used for relatively small lookups (4 - 5 items in v_Nodes and 4 calls max)
        // it will do for now
        for (UINode& uiNode : v_Nodes)
            for(const ed::PinId& id : uiNode.inputPinIDs)
                if (id == itemToSearch)
                    return &uiNode;
        return nullptr;
    }

    // Struct to hold basic information about connection between
    // pins. Note that connection (aka. link) has its own ID.
    // This is useful later with dealing with selections, deletion
    // or other operations.
    struct LinkInfo
    {
        ed::LinkId Id;
        ed::PinId  startPinId;
        ed::PinId  endPinId;
        UINode* nodeWithInputPin;
        UINode* nodeWithOutputPin;
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

    void ResetFor(AsdfAnim::Animation3D* anim)
    {
        // Check if the logic wasn't already set for this animation
        if (p_SentAnim == anim) return;

        p_SentAnim = anim;
        m_Links.clear();
        v_Nodes.clear();

        // Build the nodes from the blend tree
        uintptr_t uniqueId = 1;
        const std::vector<BlendNode*>& treeToDraw = p_SentAnim->GetBlendTree()->GetTree();
        for (size_t i = 0u; i < treeToDraw.size(); ++i)
        {
            UINode currentNode = {
                treeToDraw[i],
                uniqueId++,
                {uniqueId++, uniqueId++, uniqueId++, uniqueId++},
                {false, false, false, false},
                uniqueId++,
                0
            };
    
            switch (currentNode.animationNode->GetType())
            {
            case NodeType_::NodeType_Output:
                currentNode.Draw = [](UINode* const thisPtr, AsdfAnim::Animation3D* sentAnim) -> void {
                    ed::BeginNode(thisPtr->nodeID);
                    ImGui::Text("Output Node");
                    ed::BeginPin(thisPtr->inputPinIDs[0], ed::PinKind::Input);
                    ImGui::Text("-> In");
                    ed::EndPin();
                    ed::EndNode();
                };
                break;
            case NodeType_::NodeType_Clip:
            {
                currentNode.Draw = [](UINode* const thisPtr, AsdfAnim::Animation3D* sentAnim) -> void {
                    ed::BeginNode(thisPtr->nodeID);
                    ImGui::Text("Clip Node");
                    ImGui::BeginGroup();
                    ImGui::PushItemWidth(200);

                    const std::vector<std::string>& availableAnims = sentAnim->AvailableClips();
                    ClipNode* clipNode = reinterpret_cast<ClipNode*>(thisPtr->animationNode);
                    const char* clipName = clipNode->GetClip()->name.c_str();

                    ImGui::Text("Clip:");
                    ImGui::SameLine();
                    if (ImGui::Button(clipName))
                        ImGui::OpenPopup("clip");

                    ImGui::PopItemWidth();

                    ImGui::EndGroup();
                    ImGui::SameLine();      // Next column
                    ImGui::BeginGroup();
                    
                    ed::BeginPin(thisPtr->outputPinID, ed::PinKind::Output);
                    ImGui::Text("Out ->");
                    ed::EndPin();
                    ImGui::EndGroup();
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
                                clipNode->SetClip(sentAnim->GetClip(j));
                                ImGui::CloseCurrentPopup();
                            }
                        }
                        ImGui::EndChild();
                        ImGui::EndPopup();
                    }
                    ed::Resume();
                };
                
            }
            break;
            case NodeType_::NodeType_LinearBlend:
                currentNode.Draw = [](UINode* const thisPtr, AsdfAnim::Animation3D* sentAnim) -> void {
                    ed::BeginNode(thisPtr->nodeID);
                    ImGui::Text("Linear Blend");
                    ImGui::BeginGroup();
                    ed::BeginPin(thisPtr->inputPinIDs[0], ed::PinKind::Input);
                    ImGui::Text("-> In1");
                    ed::EndPin();
                    ed::BeginPin(thisPtr->inputPinIDs[1], ed::PinKind::Input);
                    ImGui::Text("-> In2");
                    ed::EndPin();

                    LinearBlendNode* blendNode = reinterpret_cast<LinearBlendNode*>(thisPtr->animationNode);
                    ImGui::PushItemWidth(200);
                    ImGui::SliderFloat("Blend Factor", blendNode->GetBlendValue(), 0.f, 1.f);
                    ImGui::PopItemWidth();

                    ImGui::EndGroup();
                    ImGui::SameLine();
                    ImGui::BeginGroup();

                    ed::BeginPin(thisPtr->outputPinID, ed::PinKind::Output);
                    ImGui::Text("Out ->");
                    ed::EndPin();
                    ImGui::EndGroup();
                    ed::EndNode();
                };
            break;
            case NodeType_::NodeType_LinearBlendSync:
                currentNode.Draw = [](UINode* const thisPtr, AsdfAnim::Animation3D* sentAnim) -> void {
                    ed::BeginNode(thisPtr->nodeID);
                    ImGui::Text("Linear Blend Sync");
                    ImGui::BeginGroup();
                    ed::BeginPin(thisPtr->inputPinIDs[0], ed::PinKind::Input);
                    ImGui::Text("-> In1");
                    ed::EndPin();
                    ed::BeginPin(thisPtr->inputPinIDs[1], ed::PinKind::Input);
                    ImGui::Text("-> In2");
                    ed::EndPin();

                    LinearBlendNodeSync* blendNode = reinterpret_cast<LinearBlendNodeSync*>(thisPtr->animationNode);
                    ImGui::PushItemWidth(200);
                    ImGui::SliderFloat("Blend Factor", blendNode->GetBlendValue(), 0.f, 1.f);
                    ImGui::PopItemWidth();

                    ImGui::EndGroup();
                    ImGui::SameLine();
                    ImGui::BeginGroup();

                    ed::BeginPin(thisPtr->outputPinID, ed::PinKind::Output);
                    ImGui::Text("Out ->");
                    ed::EndPin();
                    ImGui::EndGroup();
                    ed::EndNode();
                };
            break;
            default:
                break;
            }

            // Add the node
            v_Nodes.push_back(std::move(currentNode));
        }

        // Build the received links
        for (BlendNode* const& node : treeToDraw)
        {
            const std::array<BlendNode*, 4>& nodeInputs = node->GetInputs();
            for (uint8_t i = 0u; i < nodeInputs.size(); ++i)
            {
                BlendNode* const& input = nodeInputs[i];
                if (!input) continue;

                UINode* endNode = FindUINodeFromAnimationNode(node);        // This node receives the link in one of its input slots
                UINode* startNode = FindUINodeFromAnimationNode(input);     // This node starts the link from its output slot
                assert(endNode && startNode);                               // If they couldn't be found there is a logic error, it's not possible to link inexistant nodes

                // Check if no link exist with this output
                for (const auto& link : m_Links)
                {
                    // If the node with the output is the same as the start (i.e. the node with output in this link), terminate
                    if (link.nodeWithOutputPin == startNode) return;
                }

                ed::PinId startPinID, endPinID;
                // The link starts at the output of the start node
                startPinID = startNode->outputPinID;

                // The link should end at the current "i" spot. In other words, the position of the input in the nodeInputs array
                // needs to correspond to where the link ends on the node inputs
                // For example, if the input is in slot 3 of the nodeInputs array, it should be linked to the third input of the UI node
                // If the inputs slot of the UINode was already used then there is a logic error
                assert(!endNode->usedInputPinIDs[i]);
                endPinID = endNode->inputPinIDs[i];
                endNode->usedInputPinIDs[i] = true;

                // Add the link
                m_Links.push_back({ ed::LinkId(m_NextLinkId++), startPinID, endPinID, endNode, startNode });
            }
        }
    }

    void OnFrame(float deltaTime) override
    {
        auto& io = ImGui::GetIO();

        ImGui::Text("FPS: %.2f (%.2gms)", io.Framerate, io.Framerate ? 1000.0f / io.Framerate : 0.0f);

        ImGui::Separator();

        ed::SetCurrentEditor(m_Context);

        // Start interaction with editor.
        ed::Begin("My Editor", ImVec2(0.0, 0.0f));

        //
        // 1) Commit known data to editor
        //
        for(uint32_t i = 0u; i < v_Nodes.size(); ++i)
        {
            UINode& node = v_Nodes[i];
            ImGui::PushID(i);
            node.Draw(&node, p_SentAnim);
            ImGui::PopID();
        }

        // Draw Links
        for (auto& linkInfo : m_Links)
            ed::Link(linkInfo.Id, linkInfo.startPinId, linkInfo.endPinId);

        //
        // 2) Handle interactions
        //

        // Handle creation action, returns true if editor want to create new object (node or link)
        if (ed::BeginCreate())
        {
            ed::PinId startPinId, endPinId;
            if (ed::QueryNewLink(&startPinId, &endPinId))
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

                if (startPinId && endPinId) // both are valid, let's accept link
                {
                    // ed::AcceptNewItem() return true when user release mouse button.
                    if (ed::AcceptNewItem())
                    {
                        // Connect blendnodes
                        UINode* startNode = FindUINodeFromOutputPinID(startPinId);  // This node receives the link in one of its input slots
                        UINode* endNode = FindUINodeFromInputPinID(endPinId);       // This node starts the link from its output slot
                        assert(endNode && startNode);                               // If they couldn't be found there is a logic error, it's not possible to link inexistant nodes

                        for(uint8_t i = 0u; i < endNode->inputPinIDs.size(); ++i)
                            if (!endNode->usedInputPinIDs[i])
                            {
                                endNode->animationNode->SetInput(i, startNode->animationNode); // Link the two nodes
                                endNode->usedInputPinIDs[i] = true;
                                break;
                            }


                        // Since we accepted new link, lets add one to our list of links.
                        m_Links.push_back({ ed::LinkId(m_NextLinkId++), startPinId, endPinId, endNode, startNode });

                        // Draw new link.
                        ed::Link(m_Links.back().Id, m_Links.back().startPinId, m_Links.back().endPinId);
                    }

                    // You may choose to reject connection between these nodes
                    // by calling ed::RejectNewItem(). This will allow editor to give
                    // visual feedback by changing link thickness and color.
                }
                else if (startPinId)
                {

                }
                else if (endPinId)
                {

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
                            // Remove animation input
                            link.nodeWithInputPin->animationNode->RemoveInput(link.nodeWithOutputPin->animationNode);

                            // Free input slot
                            for (uint8_t i = 0u; i < link.nodeWithInputPin->inputPinIDs.size(); ++i)
                            {
                                if (link.nodeWithInputPin->inputPinIDs[i] == link.endPinId)
                                {
                                    link.nodeWithInputPin->usedInputPinIDs[i] = false;
                                    break;
                                }
                            }

                            // Delete link
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
    std::vector<UINode>     v_Nodes;
};