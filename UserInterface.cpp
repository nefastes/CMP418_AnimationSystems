#include "UserInterface.h"

UI_NodeEditor::UINode* UI_NodeEditor::FindUINodeFromAnimationNode(BlendNode* const& itemToSearch)
{
    // TERRIBLE way of doing this
    // but since this will be used for relatively small lookups (4 - 5 items in v_Nodes and 4 calls max)
    // it will do for now
    for (UINode& uiNode : v_Nodes)
        if (uiNode.animationNode == itemToSearch)
            return &uiNode;
    return nullptr;
}

UI_NodeEditor::UINode* UI_NodeEditor::FindUINodeFromOutputPinID(const ed::PinId & itemToSearch)
{
    // TERRIBLE way of doing this
    // but since this will be used for relatively small lookups (4 - 5 items in v_Nodes and 4 calls max)
    // it will do for now
    for (UINode& uiNode : v_Nodes)
        if (uiNode.outputPinID == itemToSearch)
            return &uiNode;
    return nullptr;
}

UI_NodeEditor::UINode* UI_NodeEditor::FindUINodeFromInputPinID(const ed::PinId& itemToSearch)
{
    // TERRIBLE way of doing this
    // but since this will be used for relatively small lookups (4 - 5 items in v_Nodes and 4 calls max)
    // it will do for now
    for (UINode& uiNode : v_Nodes)
        for (const ed::PinId& id : uiNode.inputPinIDs)
            if (id == itemToSearch)
                return &uiNode;
    return nullptr;
}

UI_NodeEditor::UINode* UI_NodeEditor::FindUINodeFromNodeID(const ed::NodeId& itemToSearch)
{
    // TERRIBLE way of doing this
    // but since this will be used for relatively small lookups (4 - 5 items in v_Nodes and 4 calls max)
    // it will do for now
    for (UINode& uiNode : v_Nodes)
        if (uiNode.nodeID == itemToSearch)
            return &uiNode;
    return nullptr;
}

void UI_NodeEditor::AssignDrawFunctionToUINode(UINode& node)
{
    switch (node.animationNode->GetType())
    {
    case NodeType_::NodeType_Output:
        node.Draw = [](UINode* const thisPtr, AsdfAnim::Animation3D*& sentAnim) -> void {
            ed::BeginNode(thisPtr->nodeID);
            ImGui::Text("Output Node");
            ed::BeginPin(thisPtr->inputPinIDs[0], ed::PinKind::Input);
            ImGui::Text("-> In");
            ed::EndPin();
            ed::EndNode();
        };
        break;
    case NodeType_::NodeType_Clip:
        node.Draw = [](UINode* const thisPtr, AsdfAnim::Animation3D*& sentAnim) -> void {
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
            {
                ed::Suspend();      // This gets out of the canvas coordinates and we can open the popup on screen coords instead
                ImGui::OpenPopup("clip");
                ed::Resume();
            }

            float s = clipNode->GetPlaybackSpeed();
            if (ImGui::SliderFloat("Playback Speed", &s, 0.f, 4.f))
                clipNode->SetPlaybackSpeed(s);

            bool l = clipNode->IsLooping();
            if (ImGui::Checkbox("Looping", &l))
                clipNode->SetLooping(l);

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
        break;
    case NodeType_::NodeType_LinearBlend:
        node.Draw = [](UINode* const thisPtr, AsdfAnim::Animation3D*& sentAnim) -> void {
            ed::BeginNode(thisPtr->nodeID);
            ImGui::Text("Linear Blend Node");
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
        node.Draw = [](UINode* const thisPtr, AsdfAnim::Animation3D*& sentAnim) -> void {
            ed::BeginNode(thisPtr->nodeID);
            ImGui::Text("Linear Blend Node Synchronised");
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
    case NodeType_::NodeType_Transition:
        node.Draw = [](UINode* const thisPtr, AsdfAnim::Animation3D*& sentAnim) -> void {
            ed::BeginNode(thisPtr->nodeID);
            ImGui::Text("Linear Blend Node Synchronised");
            ImGui::BeginGroup();
            ed::BeginPin(thisPtr->inputPinIDs[0], ed::PinKind::Input);
            ImGui::Text("-> In1");
            ed::EndPin();
            ed::BeginPin(thisPtr->inputPinIDs[1], ed::PinKind::Input);
            ImGui::Text("-> In2");
            ed::EndPin();

            TransitionNode* blendNode = reinterpret_cast<TransitionNode*>(thisPtr->animationNode);

            ImGui::Text("Transition Type:");
            ImGui::SameLine();
            const TransitionType_& transitionType = blendNode->GetTransitionType();
            static std::array<std::string, 5> transitionTypeNames = { "None", "Frozen", "Frozen Sync", "Smooth", "Smooth Sync"};
            if (ImGui::Button(transitionTypeNames[(size_t)transitionType + 1u].c_str()))
            {
                ed::Suspend();      // This gets out of the canvas coordinates and we can open the popup on screen coords instead
                ImGui::OpenPopup("transition");
                ed::Resume();
            }

            ImGui::PushItemWidth(200);
            float transiTime = blendNode->GetTransitionTime();
            if (ImGui::SliderFloat("Transiton Max Time", &transiTime, 0.f, 4.f))
                blendNode->SetTransitionTime(transiTime);
            ImGui::PopItemWidth();

            ImGui::PushItemWidth(100);
            bool start = blendNode->IsTransitioning();
            static std::array<std::string, 2> startStopButtonName = { "Start", "Stop" };
            if (ImGui::Button(startStopButtonName[start].c_str()))
                blendNode->ToggleTransition();

            if (ImGui::Button("Reset"))
                blendNode->Reset();
            ImGui::PopItemWidth();

            ImGui::EndGroup();
            ImGui::SameLine();
            ImGui::BeginGroup();

            ed::BeginPin(thisPtr->outputPinID, ed::PinKind::Output);
            ImGui::Text("Out ->");
            ed::EndPin();
            ImGui::EndGroup();
            ed::EndNode();

            ed::Suspend();
            if (ImGui::BeginPopup("transition")) {
                // Note: if it weren't for the child window, we would have to PushItemWidth() here to avoid a crash!
                ImGui::TextDisabled("Pick One:");
                ImGui::BeginChild("popup_scroller", ImVec2(200, 100), true, ImGuiWindowFlags_AlwaysVerticalScrollbar);
                for (size_t j = 0u; j < transitionTypeNames.size(); ++j)
                {
                    if (ImGui::Button(transitionTypeNames[j].c_str())) {
                        blendNode->SetTransitionType((TransitionType_)(j - 1));
                        ImGui::CloseCurrentPopup();
                    }
                }
                ImGui::EndChild();
                ImGui::EndPopup();
            }
            ed::Resume();
        };
        break;
    case NodeType_::NodeType_Ragdoll:
        node.Draw = [](UINode* const thisPtr, AsdfAnim::Animation3D*& sentAnim) -> void {
            ed::BeginNode(thisPtr->nodeID);
            ImGui::Text("Ragdoll Node");
            RagdollNode* node = reinterpret_cast<RagdollNode*>(thisPtr->animationNode);

            if (!node->IsRagdollValid())
            {
                ImGui::NewLine();
                ImGui::Text("No ragdoll file was found or assigned for this model.");
                ImGui::Text("Ragdoll is unavailable.");
                ed::EndNode();
                return;
            }

            ImGui::BeginGroup();
            ed::BeginPin(thisPtr->inputPinIDs[0], ed::PinKind::Input);
            ImGui::Text("-> In1");
            ed::EndPin();

            bool r = node->IsActive();
            if (ImGui::Checkbox("Activate Ragdoll", &r))
                node->SetActive(r);

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
}

void UI_NodeEditor::RemoveLink(const LinkInfo & link)
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
    v_Links.erase(&link);
}

void UI_NodeEditor::RemoveLinkWithEndNodeID(const ed::PinId & idToSearch)
{
    // Check if the endNode already had a link on this ID
    for (LinkInfo& link : v_Links)
        if (link.endPinId == idToSearch)
        {
            RemoveLink(link);
            return;
        }
}

void UI_NodeEditor::OnStop()
{
    for(std::pair<const std::string, ed::EditorContext*>& context : map_Contexts)
        ed::DestroyEditor(context.second);
    p_Context = nullptr;
}

void UI_NodeEditor::ResetFor(AsdfAnim::Animation3D* anim)
{
    // Check if the logic wasn't already set for this animation
    if (p_SentAnim == anim) return;

    // Reset context
    // This allows to keep nodes in the correct places
    std::string contextName = anim->GetFileName() + ".json";
    if (map_Contexts.find(contextName) == map_Contexts.end())
    {
        auto it = map_Contexts.insert({ std::move(contextName), nullptr }).first;   // Need to hack it a bit to reuse the same memory for th econfig file name
        ed::Config config;
        config.SettingsFile = it->first.data();
        p_Context = it->second = ed::CreateEditor(&config);
    }
    else p_Context = map_Contexts[contextName];

    // Reset data
    p_SentAnim = anim;
    v_Links.clear();
    v_Nodes.clear();
    v_Nodes.reserve(BLENDTREE_MAXNODES);

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

        // Assign a function to draw this node
        // This will be based on the BlendNode type
        AssignDrawFunctionToUINode(currentNode);

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
            for (const auto& link : v_Links)
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
            v_Links.push_back({ ed::LinkId(m_NextLinkId++), startPinID, endPinID, endNode, startNode });
        }
    }
}

void UI_NodeEditor::OnFrame(float deltaTime)
{
    auto& io = ImGui::GetIO();

    ImGui::Text("FPS: %.2f (%.2gms)", io.Framerate, io.Framerate ? 1000.0f / io.Framerate : 0.0f);

    ImGui::Separator();

    ed::SetCurrentEditor(p_Context);

    // Start interaction with editor.
    ed::Begin("My Editor", ImVec2(0.0, 0.0f));

    //
    // 1) Commit known data to editor
    //

    for (uint32_t i = 0u; i < v_Nodes.size(); ++i)
    {
        UINode& node = v_Nodes[i];
        ImGui::PushID(i);
        node.Draw(&node, p_SentAnim);
        ImGui::PopID();
    }

    // Draw Links
    for (auto& linkInfo : v_Links)
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
                    UINode* startNode = FindUINodeFromOutputPinID(startPinId);  // This node receives the link in one of its input slots
                    UINode* endNode = FindUINodeFromInputPinID(endPinId);       // This node starts the link from its output slot
                    if (!startNode && !endNode)
                    {
                        // If identifying the nodes failed, the user might have dragged from the output pin
                        startNode = FindUINodeFromOutputPinID(endPinId);
                        endNode = FindUINodeFromInputPinID(startPinId);

                        // Swap pin ids for the following logic
                        std::swap(startPinId, endPinId);
                    }
                    assert(endNode && startNode);                               // If they couldn't be found there is a logic error, it's not possible to link inexistant nodes

                    // Connect blendnodes
                    bool inputAccepted = false;
                    for (uint8_t i = 0u; i < endNode->inputPinIDs.size(); ++i)
                        if (endNode->inputPinIDs[i] == endPinId)
                        {
                            RemoveLinkWithEndNodeID(endPinId);
                            inputAccepted = endNode->animationNode->SetInput(i, startNode->animationNode); // Link the two nodes
                            endNode->usedInputPinIDs[i] = inputAccepted;
                            break;
                        }

                    if (inputAccepted)
                    {
                        // Since we accepted new link, lets add one to our list of links.
                        v_Links.push_back({ ed::LinkId(m_NextLinkId++), startPinId, endPinId, endNode, startNode });

                        // Draw new link.
                        ed::Link(v_Links.back().Id, v_Links.back().startPinId, v_Links.back().endPinId);
                    }
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
                for (auto& link : v_Links)
                {
                    if (link.Id == deletedLinkId)
                    {
                        RemoveLink(link);
                        break;
                    }
                }
            }

            // You may reject link deletion by calling:
            // ed::RejectDeletedItem();
        }
    }
    ed::EndDelete(); // Wrap up deletion action

    // Handle node creation
    ed::Suspend();
    static ed::NodeId tempNodeID = NULL;
    static ImVec2 mousePos = {0, 0};
    if (ed::ShowBackgroundContextMenu())
    {
        ImGui::OpenPopup("NCM");
        mousePos = ImGui::GetMousePos();
    }
    else if (ed::ShowNodeContextMenu(&tempNodeID))
        ImGui::OpenPopup("NDM");
    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(8, 8));
    if (ImGui::BeginPopup("NCM")) // Node Creation Menu (less characters)
    {
        if (ImGui::MenuItem("Clip Node"))
        {
            // Create a clip node
            BlendTree* blendTree = p_SentAnim->GetBlendTree();
            uint32_t nodeID = blendTree->AddNode(NodeType_::NodeType_Clip);
            ClipNode* clipNode = reinterpret_cast<ClipNode*>(blendTree->GetNode(nodeID));
            clipNode->SetClip(p_SentAnim->GetClip(0u));

            // Create the UI node
            int uniqueId = v_Nodes.back().outputPinID.Get() + 1;    // The last node has the biggest ID number in its outputPinID
            UINode currentNode = {
                clipNode,
                uniqueId++,
                {uniqueId++, uniqueId++, uniqueId++, uniqueId++},
                {false, false, false, false},
                uniqueId,
                0
            };
            AssignDrawFunctionToUINode(currentNode);
            ed::SetNodePosition(currentNode.nodeID, ed::ScreenToCanvas(mousePos));
            v_Nodes.push_back(std::move(currentNode));
            ImGui::CloseCurrentPopup();
        }
        if (ImGui::MenuItem("Linear Blend Node"))
        {
            // Create a clip node
            BlendTree* blendTree = p_SentAnim->GetBlendTree();
            uint32_t nodeID = blendTree->AddNode(NodeType_::NodeType_LinearBlend);

            // Create the UI node
            int uniqueId = v_Nodes.back().outputPinID.Get() + 1;    // The last node has the biggest ID number in its outputPinID
            UINode currentNode = {
                blendTree->GetNode(nodeID),
                uniqueId++,
                {uniqueId++, uniqueId++, uniqueId++, uniqueId++},
                {false, false, false, false},
                uniqueId,
                0
            };
            AssignDrawFunctionToUINode(currentNode);
            ed::SetNodePosition(currentNode.nodeID, ed::ScreenToCanvas(mousePos));
            v_Nodes.push_back(std::move(currentNode));
            ImGui::CloseCurrentPopup();
        }
        if (ImGui::MenuItem("Linear Blend Node Synchornised"))
        {
            // Create a clip node
            BlendTree* blendTree = p_SentAnim->GetBlendTree();
            uint32_t nodeID = blendTree->AddNode(NodeType_::NodeType_LinearBlendSync);

            // Create the UI node
            int uniqueId = v_Nodes.back().outputPinID.Get() + 1;    // The last node has the biggest ID number in its outputPinID
            UINode currentNode = {
                blendTree->GetNode(nodeID),
                uniqueId++,
                {uniqueId++, uniqueId++, uniqueId++, uniqueId++},
                {false, false, false, false},
                uniqueId,
                0
            };
            AssignDrawFunctionToUINode(currentNode);
            ed::SetNodePosition(currentNode.nodeID, ed::ScreenToCanvas(mousePos));
            v_Nodes.push_back(std::move(currentNode));
            ImGui::CloseCurrentPopup();
        }
        if (ImGui::MenuItem("Transition Node"))
        {
            // Create a transition node
            BlendTree* blendTree = p_SentAnim->GetBlendTree();
            uint32_t nodeID = blendTree->AddNode(NodeType_::NodeType_Transition);

            // Create the UI node
            int uniqueId = v_Nodes.back().outputPinID.Get() + 1;    // The last node has the biggest ID number in its outputPinID
            UINode currentNode = {
                blendTree->GetNode(nodeID),
                uniqueId++,
                {uniqueId++, uniqueId++, uniqueId++, uniqueId++},
                {false, false, false, false},
                uniqueId,
                0
            };
            AssignDrawFunctionToUINode(currentNode);
            ed::SetNodePosition(currentNode.nodeID, ed::ScreenToCanvas(mousePos));
            v_Nodes.push_back(std::move(currentNode));
            ImGui::CloseCurrentPopup();
        }
        if (ImGui::MenuItem("Ragdoll Node"))
        {
            // Create a ragdoll node
            BlendTree* blendTree = p_SentAnim->GetBlendTree();
            uint32_t nodeID = blendTree->AddNode(NodeType_::NodeType_Ragdoll);
            RagdollNode* node = reinterpret_cast<RagdollNode*>(blendTree->GetNode(nodeID));
            node->SetRagdoll(p_SentAnim->GetRagdoll());

            // Create the UI node
            int uniqueId = v_Nodes.back().outputPinID.Get() + 1;    // The last node has the biggest ID number in its outputPinID
            UINode currentNode = {
                node,
                uniqueId++,
                {uniqueId++, uniqueId++, uniqueId++, uniqueId++},
                {false, false, false, false},
                uniqueId,
                0
            };
            AssignDrawFunctionToUINode(currentNode);
            ed::SetNodePosition(currentNode.nodeID, ed::ScreenToCanvas(mousePos));
            v_Nodes.push_back(std::move(currentNode));
            ImGui::CloseCurrentPopup();
        }
        ImGui::EndPopup();
    }
    if (ImGui::BeginPopup("NDM")) // Node Deletion Menu (less characters)
    {
        assert(tempNodeID); // If this popup is opened without a nodeID there is a logic error
        UINode* node = FindUINodeFromNodeID(tempNodeID);

        // Do not allow to delete the output node
        const bool isOutputNode = node->animationNode->GetType() == NodeType_::NodeType_Output;
        if (isOutputNode)
            ImGui::Text("The output node cannot be deleted!");
        else if (ImGui::MenuItem("Delete Node"))
        {
            // Remove any link with this node
            // Find any link with a start ID that corresponds to this node's output ID
            // Find any link with an end ID that corresponds to any of this node's input IDs
            for (int i = 0; i < v_Links.size(); ++i)
            {
                LinkInfo& link = v_Links[i];
                // Do not break if a link is found as there might be more
                if (link.startPinId == node->outputPinID)
                {
                    RemoveLink(link);

                    // Removing this link rearranges the vector, thus the current item[i] can at this point be what would have been item[i+1]
                    --i;
                    continue;
                }
                for (const ed::PinId& inputPinID : node->inputPinIDs)
                    if (link.endPinId == inputPinID)
                    {
                        RemoveLink(link);
                        --i;
                        break;
                    }
            }

            // Remove the node
            p_SentAnim->GetBlendTree()->RemoveAndFreeNode(node->animationNode);
            // TODO: Replace with ImVector and use v_Node.erase(node);
            for (auto it = v_Nodes.begin(); it != v_Nodes.end(); ++it)
                if (&*it == node)
                {
                    v_Nodes.erase(it);
                    break;
                }

            ImGui::CloseCurrentPopup();
        }
        ImGui::EndPopup();
    }
    ImGui::PopStyleVar();
    ed::Resume();

    // End of interaction with editor.
    ed::End();

    if (m_FirstFrame)
        ed::NavigateToContent(0.0f);

    ed::SetCurrentEditor(nullptr);

    m_FirstFrame = false;

    // ImGui::ShowMetricsWindow();
}
