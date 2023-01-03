#include "BlendNode.h"
#include "animation/animation.h"
using namespace AsdfAnim;

BlendNode::BlendNode(const gef::SkeletonPose& bindPose) : a_Inputs{nullptr}, r_BindPose(bindPose), m_BlendedPose(bindPose), m_Type(NodeType_::NodeType_Undefined)
{
}

void BlendNode::AddInput(BlendNode * input)
{
	for (auto& item : a_Inputs)
		if (item == nullptr)
		{
			item = input;
			break;
		}
}

void BlendNode::RemoveInput(BlendNode * input)
{
	for(auto& item : a_Inputs)
		if (item == input)
		{
			item = nullptr;
			break;
		}
}

bool BlendNode::Update(float frameTime, bool& needPhysicsUpdate)
{
	// Track and update all the parameters before processing its own data to follow a post-order traversal
	bool success = true;
	for (uint32_t i = 0u; i < a_Inputs.size(); ++i)
	{
		// Recursively call the update of each input node
		// This will create a post-order traversal on the entire tree
		// The last nodes of the tree will have no parameter to update
		if (a_Inputs[i]) success &= a_Inputs[i]->Update(frameTime, needPhysicsUpdate);
	}

	if (success)	return ProcessData(frameTime);
	else			return false;
}

/// <summary>
/// Output
/// </summary>
/// <param name="bindPose"></param>
OutputNode::OutputNode(const gef::SkeletonPose& bindPose) : BlendNode(bindPose)
{
	m_Type = NodeType_::NodeType_Output;
}

bool OutputNode::ProcessData(float frameTime)
{
	// Pass the input pose as the final output pose
	// This node should only have one input
	if (!a_Inputs[0]) return false;

	m_BlendedPose = a_Inputs[0]->GetPose();
	return true;
}

/// <summary>
/// Clip
/// </summary>
/// <param name="bindPose"></param>
ClipNode::ClipNode(const gef::SkeletonPose& bindPose) : BlendNode(bindPose), m_AnimationTime(0.f), m_ClipPlaybackSpeed(1.f), m_ClipLooping(true), p_Clip(nullptr)
{
	m_Type = NodeType_::NodeType_Clip;
}

bool ClipNode::ProcessData(float frameTime)
{
	bool finished = false;

	if (p_Clip)
	{
		gef::Animation* gefClip = p_Clip->clip;
		// update the animation playback time
		m_AnimationTime += frameTime * m_ClipPlaybackSpeed;

		// check to see if the playback has reached the end of the animation
		if (m_AnimationTime > gefClip->duration())
		{
			// if the animation is looping then wrap the playback time round to the beginning of the animation
			// other wise set the playback time to the end of the animation and flag that we have reached the end
			if (m_ClipLooping)
				m_AnimationTime = std::fmodf(m_AnimationTime, gefClip->duration());
			else
			{
				m_AnimationTime = gefClip->duration();
				finished = true;
			}
		}

		// add the clip start time to the playback time to calculate the final time
		// that will be used to sample the animation data
		float time = m_AnimationTime + gefClip->start_time();

		// sample the animation data at the calculated time
		// any bones that don't have animation data are set to the bind pose
		m_BlendedPose.SetPoseFromAnim(*gefClip, r_BindPose, time);
	}
	else
	{
		// no animation associated with this player
		// just set the pose to the bind pose
		m_BlendedPose = r_BindPose;
	}

	// return true if we have reached the end of the animation, always false when playback is looped
	return !finished;
}

/// <summary>
/// Linear Blend
/// </summary>
/// <param name="bindPose"></param>
LinearBlendNode::LinearBlendNode(const gef::SkeletonPose& bindPose) : BlendNode(bindPose), m_BlendValue(0.f)
{
	m_Type = NodeType_::NodeType_LinearBlend;
}

bool LinearBlendNode::ProcessData(float frameTime)
{
	// This blend node only process the two first inputs
	if (a_Inputs[0] && a_Inputs[1])			m_BlendedPose.Linear2PoseBlend(a_Inputs[0]->GetPose(), a_Inputs[1]->GetPose(), m_BlendValue);
	else if (!a_Inputs[0] && a_Inputs[1])	m_BlendedPose = a_Inputs[1]->GetPose();
	else if (a_Inputs[0] && !a_Inputs[1])	m_BlendedPose = a_Inputs[0]->GetPose();
	else									return false;
	return true;
}

/// <summary>
/// Linear Blend Synchronised
/// Scales the clips durations to be synchronised, ideal for walk <-> run animations
/// </summary>
/// <param name="bindPose"></param>
LinearBlendNodeSync::LinearBlendNodeSync(const gef::SkeletonPose& bindPose) : LinearBlendNode(bindPose),
a_ClipTypes{AsdfAnim::ClipType::Clip_Type_Undefined, AsdfAnim::ClipType::Clip_Type_Undefined },
a_ClipsMaxMin{0.f, 0.f}
{
	m_Type = NodeType_::NodeType_LinearBlendSync;
}

bool LinearBlendNodeSync::ProcessData(float frameTime)
{
	// Scale the two input clips to be the same lengh
	if (a_Inputs[0] && a_Inputs[1])			CalculateBlendedSyncPose();
	else if (!a_Inputs[0] && a_Inputs[1])	m_BlendedPose = a_Inputs[1]->GetPose();
	else if (a_Inputs[0] && !a_Inputs[1])	m_BlendedPose = a_Inputs[0]->GetPose();
	else return false;

	return true;
}

bool LinearBlendNodeSync::SetInput(uint32_t slot, BlendNode* input)
{
	// Refuse any input that is not a clip
	if (input->GetType() != NodeType_::NodeType_Clip) return false;

	BlendNode::SetInput(slot, input);

	// Whenever an input is added to this node we need to resync the clips
	for (BlendNode* node : a_Inputs)
		if (node)
			reinterpret_cast<ClipNode*>(node)->ResetAnimationTime();
	reinterpret_cast<ClipNode*>(input)->ResetAnimationTime();

	// When there are two inputs, it means the node can operate
	// Take advantage to do only-once initialisations
	if (a_Inputs[0] && a_Inputs[1])
	{
		const ClipNode* input1 = reinterpret_cast<ClipNode*>(a_Inputs[0]);
		const ClipNode* input2 = reinterpret_cast<ClipNode*>(a_Inputs[1]);
		a_ClipTypes = { input1->GetClip()->type, input2->GetClip()->type };
		CalculateClipsMaxMin();
	}

	// Input accepted
	return true;
}

void LinearBlendNodeSync::CalculateClipsMaxMin()
{
	// Initialise the clip1 and clip2 playback speed min and max respectively for a synchronised blend
	// array = { clip1_maxSpeed, clip2_minSpeed }
	// clip1_maxSpeed determines the maximum speed clip1 needs to be at when clip2 is at normal speed (1)
	// clip2_minSpeed determines the minimum speed clip2 needs to be at when clip1 is at normal speed (1)
	ClipNode* input1 = reinterpret_cast<ClipNode*>(a_Inputs[0]);
	ClipNode* input2 = reinterpret_cast<ClipNode*>(a_Inputs[1]);
	const float duration1 = input1->GetClip()->clip->duration();
	const float duration2 = input2->GetClip()->clip->duration();
	a_ClipsMaxMin = { duration1 / duration2, duration2 / duration1 };
}

void LinearBlendNodeSync::CalculateBlendedSyncPose()
{
	ClipNode* input1 = reinterpret_cast<ClipNode*>(a_Inputs[0]);
	ClipNode* input2 = reinterpret_cast<ClipNode*>(a_Inputs[1]);

	// Check if the clips have changed, in which case the maxmin needs to be recalculated
	const AsdfAnim::ClipType input_type1 = input1->GetClip()->type;
	const AsdfAnim::ClipType input_type2 = input2->GetClip()->type;
	if (input_type1 != a_ClipTypes[0] || input_type2 != a_ClipTypes[1])
	{
		a_ClipTypes = { input_type1, input_type2 };
		CalculateClipsMaxMin();
	}

	const float input1_mod = (a_ClipsMaxMin[0] - 1.f) * m_BlendValue;	// With mock values: 1.38 - 1 * 0.5 -> 0.38 * 0.5 -> 38 percent speed, but halfed cause of blend
	const float input2_mod = (1.f - a_ClipsMaxMin[1]) * m_BlendValue;	// 

	input1->SetPlaybackSpeed(1.f + input1_mod);							// With mock values: 1
	input2->SetPlaybackSpeed(a_ClipsMaxMin[1] + input2_mod);			// 

	m_BlendedPose.Linear2PoseBlend(a_Inputs[0]->GetPose(), a_Inputs[1]->GetPose(), m_BlendValue);
}

///
/// Transition Node
/// 
TransitionNode::TransitionNode(const gef::SkeletonPose & bindPose) : LinearBlendNodeSync(bindPose),
m_TransitionType(TransitionType_::TransitionType_Undefined),
m_Transitioning(false),
m_TransitionTime(1.f),
m_CurrentTime(0.f)
{
	m_Type = NodeType_::NodeType_Transition;
}

bool TransitionNode::Update(float frameTime, bool & needPhysicsUpdate)
{
	// Track and update all the parameters before processing its own data to follow a post-order traversal
	bool success = true;

	if (a_Inputs[0] && a_Inputs[1])
	{
		// Needs to transition from input1 to input2 within the transition time set
		// Time: 0 <= m_CurrentTime <= m_TransitionTime
		if (m_Transitioning)
		{
			m_CurrentTime += frameTime;
			m_Transitioning = m_CurrentTime < m_TransitionTime;	// Stop if over endTime

			switch (m_TransitionType)
			{
			case TransitionType_::TransitionType_Smooth:
			case TransitionType_::TransitionType_Smooth_Sync:
				// The smooth transition updates both clips
				success &= a_Inputs[0]->Update(frameTime, needPhysicsUpdate);
			case TransitionType_::TransitionType_Frozen:
			case TransitionType_::TransitionType_Frozen_Sync:
				success &= a_Inputs[1]->Update(frameTime, needPhysicsUpdate);
				break;
			default:
				// No transition, so just update the first input
				success &= a_Inputs[0]->Update(frameTime, needPhysicsUpdate);
				break;
			}
		}
		// If the transition completed
		else if (m_CurrentTime > 0.f) success &= a_Inputs[1]->Update(frameTime, needPhysicsUpdate);
		// If the transition has not yet begun
		else success &= a_Inputs[0]->Update(frameTime, needPhysicsUpdate);
	}
	else if (!a_Inputs[0] && a_Inputs[1])	success &= a_Inputs[1]->Update(frameTime, needPhysicsUpdate);
	else if (a_Inputs[0] && !a_Inputs[1])	success &= a_Inputs[0]->Update(frameTime, needPhysicsUpdate);
	else return false;

	if (success)	return ProcessData(frameTime);
	else			return false;
}

bool TransitionNode::ProcessData(float frameTime)
{
	// Check inputs
	if (a_Inputs[0] && a_Inputs[1])
	{
		// Needs to transition from input1 to input2 within the transition time set
		// Time: 0 <= m_CurrentTime <= m_TransitionTime
		if (m_Transitioning)
		{
			// If the transition should stop, blendVal = 1. Otherwise blendVal = currTime / tranTime
			m_BlendValue = !m_Transitioning + m_Transitioning * (m_CurrentTime / m_TransitionTime);

			switch (m_TransitionType)
			{
			case TransitionType_::TransitionType_Smooth:
			case TransitionType_::TransitionType_Frozen:
				m_BlendedPose.Linear2PoseBlend(a_Inputs[0]->GetPose(), a_Inputs[1]->GetPose(), m_BlendValue);
				break;
			case TransitionType_::TransitionType_Smooth_Sync:
			case TransitionType_::TransitionType_Frozen_Sync:
				CalculateBlendedSyncPose();
				break;
			default:
				// Undefined
				m_BlendedPose = a_Inputs[0]->GetPose();
				break;
			}
		}
		// If the transition completed
		else if(m_CurrentTime > 0.f) m_BlendedPose = m_BlendedPose = a_Inputs[1]->GetPose();
		// If the transition has not yet begun
		else m_BlendedPose = a_Inputs[0]->GetPose();
	}
	else if (!a_Inputs[0] && a_Inputs[1])	m_BlendedPose = a_Inputs[1]->GetPose();
	else if (a_Inputs[0] && !a_Inputs[1])	m_BlendedPose = a_Inputs[0]->GetPose();
	else return false;

	return true;
}

bool TransitionNode::SetInput(uint32_t slot, BlendNode* input)
{
	// Refuse any input that is not a clip
	if (input->GetType() != NodeType_::NodeType_Clip) return false;

	return BlendNode::SetInput(slot, input);
}

void TransitionNode::StartTransition()
{
	// If no transition was defined, make it instant
	if (m_TransitionType == TransitionType_::TransitionType_Undefined)
	{
		m_CurrentTime = m_TransitionTime;
		return;
	}

	Reset();

	// If the transition is synchronised, align the second input clock to the first one
	if (m_TransitionType == TransitionType_::TransitionType_Frozen_Sync || m_TransitionType == TransitionType_::TransitionType_Smooth_Sync)
	{
		ClipNode* clip1 = reinterpret_cast<ClipNode*>(a_Inputs[0]);
		ClipNode* clip2 = reinterpret_cast<ClipNode*>(a_Inputs[1]);
		clip2->SetAnimationTime(clip1->GetAnimationTime());
		a_ClipTypes = { clip1->GetClip()->type, clip2->GetClip()->type };
		CalculateClipsMaxMin();
	}

	// Only start a transition if two inputs are available
	m_Transitioning = a_Inputs[0] && a_Inputs[1];
}

void TransitionNode::Reset()
{
	m_CurrentTime = 0.f;

	// Reset the clip playback speeds if they have been tweaked by a synchronised transition
	if (a_Inputs[0]) reinterpret_cast<ClipNode*>(a_Inputs[0])->SetPlaybackSpeed(1.f);
	if (a_Inputs[1]) reinterpret_cast<ClipNode*>(a_Inputs[1])->SetPlaybackSpeed(1.f);
}

///
/// Ragdoll Node
/// 
RagdollNode::RagdollNode(const gef::SkeletonPose & bindPose) : BlendNode(bindPose), p_Ragdoll(nullptr), m_Active(true)
{
	m_Type = NodeType_::NodeType_Ragdoll;
}

bool RagdollNode::Update(float frameTime, bool & needPhysicsUpdate)
{
	// Track and update all the parameters before processing its own data to follow a post-order traversal
	bool success = true;

	// TODO: This is dumb cause there is only one input but I'll leave it in case it changes in the future
	for (uint32_t i = 0u; i < a_Inputs.size(); ++i)
	{
		// Recursively call the update of each input node
		// This will create a post-order traversal on the entire tree
		// The last nodes of the tree will have no parameter to update
		if (a_Inputs[i]) success &= a_Inputs[i]->Update(frameTime, needPhysicsUpdate);
	}

	// Ragdoll nodes will need a physics iteration
	needPhysicsUpdate = m_Active;

	if (success)	return ProcessData(frameTime);
	else			return false;
}

bool RagdollNode::ProcessData(float frameTime)
{
	if (!p_Ragdoll) return false;

	// If the node is deactivated and there is an input, update the ragdoll according to the input
	if (!m_Active && a_Inputs[0])
	{
		m_BlendedPose = a_Inputs[0]->GetPose();
		p_Ragdoll->set_pose(m_BlendedPose);
		p_Ragdoll->UpdateRagdollFromPose();
	}
	else
	{
		p_Ragdoll->UpdatePoseFromRagdoll();
		m_BlendedPose = p_Ragdoll->pose();
	}
	return true;
}

/// <summary>
/// Blend tree
/// </summary>
/// <param name="bindPose"></param>
BlendTree::BlendTree(const gef::SkeletonPose& bindPose) : m_BindPose(bindPose)
{
	// The root node will always be an output node
	v_Tree.reserve(BLENDTREE_MAXNODES);
	v_Tree.push_back(new OutputNode(m_BindPose));
}

BlendTree::~BlendTree()
{
	for (auto& item : v_Tree)
		delete item, item = nullptr;
	v_Tree.clear();
}

uint32_t BlendTree::AddNode(BlendNode* node)
{
	// Do not allow to add a node if we reached BLENDTREE_MAXNODES
	if (v_Tree.size() >= BLENDTREE_MAXNODES)
		return UINT32_MAX;

	v_Tree.push_back(node);
	return v_Tree.size() - 1u;
}

uint32_t BlendTree::AddNode(NodeType_ type)
{
	// Do not allow to add a node if we reached BLENDTREE_MAXNODES
	if (v_Tree.size() >= BLENDTREE_MAXNODES)
		return UINT32_MAX;

	switch (type)
	{
	case NodeType_::NodeType_Output:			v_Tree.push_back(new OutputNode(m_BindPose));				break;
	case NodeType_::NodeType_Clip:				v_Tree.push_back(new ClipNode(m_BindPose));					break;
	case NodeType_::NodeType_LinearBlend:		v_Tree.push_back(new LinearBlendNode(m_BindPose));			break;
	case NodeType_::NodeType_LinearBlendSync:	v_Tree.push_back(new LinearBlendNodeSync(m_BindPose));		break;
	case NodeType_::NodeType_Transition:		v_Tree.push_back(new TransitionNode(m_BindPose));			break;
	case NodeType_::NodeType_Ragdoll:			v_Tree.push_back(new RagdollNode(m_BindPose));				break;
	default:
		throw std::logic_error("Tried to create a non-existant node!");
	}
	return v_Tree.size() - 1u;
}

void BlendTree::RemoveAndFreeNode(BlendNode* node)
{
	// Ouch
	for(auto it = v_Tree.begin(); it != v_Tree.end(); ++it)
		if (*it == node)
		{
			v_Tree.erase(it);
			delete node;
			node = nullptr;
			break;
		}
}

BlendNode* BlendTree::GetNode(uint32_t ID)
{
	return v_Tree[ID];
}

void BlendTree::ConnectToRoot(uint32_t inputNodeID)
{
	v_Tree[0u]->SetInput(0u, v_Tree[inputNodeID]);
}

void BlendTree::Update(float frameTime, bool& needsPhysicsUpdate)
{
	// Call the update on the root node to commence a post-order traversal
	v_Tree[0]->Update(frameTime, needsPhysicsUpdate);
}