#include "BlendNode.h"
#include "animation/animation.h"
BlendNode::BlendNode(const gef::SkeletonPose& bindPose) : a_Inputs{nullptr}, p_BindPose(bindPose)
{
}

template<typename...B> void BlendNode::SetInputs(B*...args)
{
	uint32_t index = 0u;
	((a_Inputs[index] = args, ++index), ...);
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
	m_BlendedPose = std::move(const_cast<gef::SkeletonPose&>(a_Inputs[0]->GetPose()));
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
		m_BlendedPose.SetPoseFromAnim(*p_Clip, p_BindPose, time);
	}
	else
	{
		// no animation associated with this player
		// just set the pose to the bind pose
		m_BlendedPose = p_BindPose;
	}

	// return true if we have reached the end of the animation, always false when playback is looped
	return finished;
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
LinearBlendNode::LinearBlendNode(const gef::SkeletonPose& bindPose) : BlendNode(bindPose), m_BlendValue(0.f)
{
}

bool LinearBlendNode::ProcessData(float frameTime)
{
	// This blend node only process the two first inputs
	m_BlendedPose.Linear2PoseBlend(a_Inputs[0]->GetPose(), a_Inputs[1]->GetPose(), m_BlendValue);
	return true;
}

/// <summary>
/// Blend tree
/// </summary>
/// <param name="bindPose"></param>
BlendTree::BlendTree(const gef::SkeletonPose& bindPose) : m_BindPose(bindPose)
{
	// The root node will always be an output node
	m_Tree.push_back(new OutputNode(m_BindPose));
}

BlendTree::~BlendTree()
{
	for (auto& item : m_Tree)
		delete item, item = nullptr;
	m_Tree.clear();
}

const gef::SkeletonPose& BlendTree::GetOutputPose()
{
	// Return the pose from the output node
	return m_Tree.front()->GetPose();
}

uint32_t BlendTree::AddNode(BlendNode* node)
{
	m_Tree.push_back(node);
	return static_cast<uint32_t>(m_Tree.size() - 1u);
}

BlendNode* BlendTree::GetNode(uint32_t index)
{
	return m_Tree[index];
}

void BlendTree::Update(float frameTime)
{
	// Call the update on the root node to commence a post-order traversal
	m_Tree[0]->Update(frameTime);
}
