#include "BlendNode.h"
#include "animation/animation.h"
BlendNode::BlendNode(const gef::SkeletonPose& bindPose) : a_Inputs{nullptr}, r_BindPose(bindPose), m_BlendedPose(bindPose)
{
}

void BlendNode::SetInput(uint32_t slot, BlendNode* input)
{
	a_Inputs[slot] = input;
}

void BlendNode::SetInput(BlendNode* input1, BlendNode* input2)
{
	a_Inputs = { input1, input2, nullptr, nullptr };
}

void BlendNode::SetInput(BlendNode* input1, BlendNode* input2, BlendNode* input3, BlendNode* input4)
{
	a_Inputs = { input1, input2, input3, input4 };
}

bool BlendNode::Update(float frameTime)
{
	// Track and update all the parameters before processing its own data to follow a post-order traversal
	bool success = true;
	for (uint32_t i = 0u; i < a_Inputs.size(); ++i)
	{
		// Recursively call the update of each input node
		// This will create a post-order traversal on the entire tree
		// The last nodes of the tree will have no parameter to update
		if (a_Inputs[i]) success &= a_Inputs[i]->Update(frameTime);
	}

	if (success)	return ProcessData(frameTime);
	else			return false;
}

const gef::SkeletonPose& BlendNode::GetPose()
{
	return m_BlendedPose;
}

/// <summary>
/// Output
/// </summary>
/// <param name="bindPose"></param>
OutputNode::OutputNode(const gef::SkeletonPose& bindPose) : BlendNode(bindPose)
{
}

bool OutputNode::ProcessData(float frameTime)
{
	// Pass the input pose as the final output pose
	// This node should only have one input
	m_BlendedPose = a_Inputs[0]->GetPose();
	return true;
}

/// <summary>
/// Clip
/// </summary>
/// <param name="bindPose"></param>
ClipNode::ClipNode(const gef::SkeletonPose& bindPose) : BlendNode(bindPose), m_AnimationTime(0.f), m_ClipPlaybackSpeed(1.f), m_ClipLooping(true), p_Clip(nullptr)
{
}

bool ClipNode::ProcessData(float frameTime)
{
	bool finished = false;

	if (p_Clip)
	{
		// update the animation playback time
		m_AnimationTime += frameTime * m_ClipPlaybackSpeed;

		// check to see if the playback has reached the end of the animation
		if (m_AnimationTime > p_Clip->duration())
		{
			// if the animation is looping then wrap the playback time round to the beginning of the animation
			// other wise set the playback time to the end of the animation and flag that we have reached the end
			if (m_ClipLooping)
				m_AnimationTime = std::fmodf(m_AnimationTime, p_Clip->duration());
			else
			{
				m_AnimationTime = p_Clip->duration();
				finished = true;
			}
		}

		// add the clip start time to the playback time to calculate the final time
		// that will be used to sample the animation data
		float time = m_AnimationTime + p_Clip->start_time();

		// sample the animation data at the calculated time
		// any bones that don't have animation data are set to the bind pose
		m_BlendedPose.SetPoseFromAnim(*p_Clip, r_BindPose, time);
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

void ClipNode::SetPlaybackSpeed(float speed)
{
	m_ClipPlaybackSpeed = speed;
}
void ClipNode::SetLooping(bool loop)
{
	m_ClipLooping = loop;
}
void ClipNode::SetClip(const gef::Animation* clip)
{
	p_Clip = clip;
}
float ClipNode::GetPlaybackSpeed()
{
	return m_ClipPlaybackSpeed;
}
bool ClipNode::IsLooping()
{
	return m_ClipLooping;
}
const gef::Animation* ClipNode::GetClip()
{
	return p_Clip;
}

/// <summary>
/// Linear Blend
/// </summary>
/// <param name="bindPose"></param>
LinearBlendNode::LinearBlendNode(const gef::SkeletonPose& bindPose) : BlendNode(bindPose), p_BlendValue(nullptr)
{
}

bool LinearBlendNode::ProcessData(float frameTime)
{
	// This blend node only process the two first inputs
	if (!p_BlendValue || !a_Inputs[0] || !a_Inputs[1])
		return false;

	m_BlendedPose.Linear2PoseBlend(a_Inputs[0]->GetPose(), a_Inputs[1]->GetPose(), *p_BlendValue);
	return true;
}

void LinearBlendNode::SetBlendValuePtr(float* pBlendVal)
{
	p_BlendValue = pBlendVal;
}

/// <summary>
/// Linear Blend Synchronised
/// Scales the clips durations to be synchronised, ideal for walk <-> run animations
/// </summary>
/// <param name="bindPose"></param>
LinearBlendNodeSync::LinearBlendNodeSync(const gef::SkeletonPose& bindPose) : LinearBlendNode(bindPose)
{
}

bool LinearBlendNodeSync::ProcessData(float frameTime)
{
	// Scale the two input clips to be the same lengh
	if (!p_BlendValue || !a_Inputs[0] || !a_Inputs[1])
		return false;

	ClipNode* input1 = reinterpret_cast<ClipNode*>(a_Inputs[0]);
	ClipNode* input2 = reinterpret_cast<ClipNode*>(a_Inputs[1]);

	float duration1 = input1->GetClip()->duration() / input2->GetClip()->duration() - 1.f;
	float duration2 = 1.f - input2->GetClip()->duration() / input1->GetClip()->duration();

	input1->SetPlaybackSpeed(duration1 * *p_BlendValue);
	input2->SetPlaybackSpeed(duration2 * *p_BlendValue);

	m_BlendedPose.Linear2PoseBlend(a_Inputs[0]->GetPose(), a_Inputs[1]->GetPose(), *p_BlendValue);

	return LinearBlendNode::ProcessData(frameTime);
}

/// <summary>
/// Blend tree
/// </summary>
/// <param name="bindPose"></param>
BlendTree::BlendTree(const gef::SkeletonPose& bindPose) : m_BindPose(bindPose)
{
	// The root node will always be an output node
	v_Tree.push_back(new OutputNode(m_BindPose));
}

BlendTree::~BlendTree()
{
	for (auto& item : v_Tree)
		delete item, item = nullptr;
	v_Tree.clear();
}

const gef::SkeletonPose& BlendTree::GetOutputPose()
{
	// Return the pose from the output node
	return v_Tree.front()->GetPose();
}

uint32_t BlendTree::AddNode(BlendNode* node)
{
	v_Tree.push_back(node);
	return v_Tree.size() - 1u;
}

uint32_t BlendTree::AddNode(NodeType_ type)
{
	switch (type)
	{
	case NodeType_::NodeType_Output:		v_Tree.push_back(new OutputNode(m_BindPose));		break;
	case NodeType_::NodeType_Clip:			v_Tree.push_back(new ClipNode(m_BindPose));			break;
	case NodeType_::NodeType_LinearBlend:	v_Tree.push_back(new LinearBlendNode(m_BindPose));	break;
	default:
		return UINT32_MAX;
	}
	return v_Tree.size() - 1u;
}

BlendNode* BlendTree::GetNode(uint32_t ID)
{
	return v_Tree[ID];
}

void BlendTree::ConnectToRoot(uint32_t inputNodeID)
{
	v_Tree[0u]->SetInput(0u, v_Tree[inputNodeID]);
}

void BlendTree::ConnectNode(uint32_t inputNodeID, uint32_t inputSlot, uint32_t receiverNodeID)
{
	// No check on these functions because the parameters should only be optained using the AddNode function
	// The input slot should be dictated by the GUI
	v_Tree[receiverNodeID]->SetInput(inputSlot, v_Tree[inputNodeID]);
}

void BlendTree::ConnectNodes(uint32_t inputNodeID1, uint32_t inputNodeID2, uint32_t receiverNodeID)
{
	v_Tree[receiverNodeID]->SetInput(v_Tree[inputNodeID1], v_Tree[inputNodeID2]);
}

void BlendTree::ConnectNodes(uint32_t inputNodeID1, uint32_t inputNodeID2, uint32_t inputNodeID3, uint32_t inputNodeID4, uint32_t receiverNodeID)
{
	v_Tree[receiverNodeID]->SetInput(v_Tree[inputNodeID1], v_Tree[inputNodeID2], v_Tree[inputNodeID3], v_Tree[inputNodeID4]);
}

void BlendTree::Update(float frameTime)
{
	// Call the update on the root node to commence a post-order traversal
	v_Tree[0]->Update(frameTime);
}
