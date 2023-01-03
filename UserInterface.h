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
// - Code cleanup, remove unecessary things and move one-line functions to headers
// 
// IF THERE IS TIME LEFTOVER
// - 2D animation JSON to binary converter
// - Add a blend tree for 2D rigged animations
// - Add support for the node editor with 2D animations
// - Figure out how ImVector works properly and maybe take advantage of it to change it for v_Nodes
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
        AsdfAnim::BlendNode* animationNode;
        ed::NodeId nodeID;
        std::array<ed::PinId, 4> inputPinIDs;
        ed::PinId outputPinID;
        std::function<void(UINode* const thisPtr, AsdfAnim::Animation3D*& sentAnim)> Draw;
    };
    UINode* FindUINodeFromAnimationNode(AsdfAnim::BlendNode* const& itemToSearch);
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
    void OnStop() final override;
    void ResetFor(AsdfAnim::Animation3D* anim);
    void OnFrame(float deltaTime) final override;

    // Data
    ed::EditorContext*      p_Context = nullptr;    // Editor context, required to trace a editor state.
    bool                    m_FirstFrame = true;    // Flag set for first frame only, some action need to be executed once.
    ImVector<LinkInfo>      v_Links;                // List of live links. It is dynamic unless you want to create read-only view over nodes.
    int                     m_NextLinkId = 100;     // Counter to help generate link ids. In real application this will probably based on pointer to user data structure.
    AsdfAnim::Animation3D*  p_SentAnim = nullptr;
    std::vector<UINode>     v_Nodes;
    std::unordered_map<std::string, ed::EditorContext*> map_Contexts;
};