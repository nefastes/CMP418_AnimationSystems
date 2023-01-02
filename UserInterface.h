#pragma once
# include "imgui.h"
# include "node-editor\imgui_node_editor.h"
# include "node-editor\application.h"
#include "Animation3D.h"
#include <unordered_map>
#include <functional>
#include <array>

namespace ed = ax::NodeEditor;

/////TODO
// - BUG: ResetFor does not reset node placement. I.E. the output node is in the same position for all tabs
// - I had an occurence of spawning a node with a link, can't reproduce
// - Maybe put a pointer of the sent anim to UINode to avoid an extra parameter? Or ref the pointer in the Draw()
// - Get rid of usedInputPinIDs ?
// - Put BlenNodes inside namespace AsdfAnim::
// 
// IF THERE IS TIME LEFTOVER
// - 2D animation JSON to binary converter
// - Figure out how ImVector works properly and maybe take advantage of it to change it for v_Nodes
// - Add a blend tree for 2D rigged animations
// - Add support for the node editor with 2D animations
////

struct UI_NodeEditor :
    public Application
{
    using Application::Application;

    // A structure that describes a node to be displayed on the canvas
    // This serves as a link between the animation nodes and the UI nodes
    // Below are several utility functions tied to this structure
    struct UINode
    {
        BlendNode* animationNode;
        ed::NodeId nodeID;
        std::array<ed::PinId, 4> inputPinIDs;
        std::array<bool, 4> usedInputPinIDs;
        ed::PinId outputPinID;
        std::function<void(UINode* const thisPtr, AsdfAnim::Animation3D*& sentAnim)> Draw;
    };
    UINode* FindUINodeFromAnimationNode(BlendNode* const& itemToSearch);
    UINode* FindUINodeFromOutputPinID(const ed::PinId& itemToSearch);
    UINode* FindUINodeFromInputPinID(const ed::PinId& itemToSearch);
    UINode* FindUINodeFromNodeID(const ed::NodeId& itemToSearch);
    void AssignDrawFunctionToUINode(UINode& node);

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
    void RemoveLink(const LinkInfo& link);
    void RemoveLinkWithEndNodeID(const ed::PinId& idToSearch);

    // Editor main functions
    // These are used to create, run, reset and destroy the editor properly
    void OnStart() override;
    void OnStop() override;
    void ResetFor(AsdfAnim::Animation3D* anim);
    void OnFrame(float deltaTime) override;

    // Data
    ed::EditorContext*      p_Context = nullptr;    // Editor context, required to trace a editor state.
    bool                    m_FirstFrame = true;    // Flag set for first frame only, some action need to be executed once.
    ImVector<LinkInfo>      v_Links;                // List of live links. It is dynamic unless you want to create read-only view over nodes.
    int                     m_NextLinkId = 100;     // Counter to help generate link ids. In real application this will probably based on pointer to user data structure.
    AsdfAnim::Animation3D*  p_SentAnim = nullptr;
    std::vector<UINode>     v_Nodes;
};