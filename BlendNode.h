#pragma once
#include <stdint.h>
#include <array>
#include "animation/skeleton.h"
class BlendNode
{
public:
	BlendNode(const gef::SkeletonPose& bindPose);
	void SetInput(uint32_t slot, BlendNode* input);
	void SetInput(BlendNode* input1, BlendNode* input2);
	void SetInput(BlendNode* input1, BlendNode* input2, BlendNode* input3, BlendNode* input4);
	bool Update(float frameTime);
	virtual bool ProcessData(float frameTime) = 0;
	const gef::SkeletonPose& GetPose();

protected:
	std::array<BlendNode*, 4> a_Inputs; // Shouldnt need more than 4 inputs
	const gef::SkeletonPose& r_BindPose;
	gef::SkeletonPose m_BlendedPose;
};

struct OutputNode : public BlendNode
{
	OutputNode(const gef::SkeletonPose& bindPose);
	bool ProcessData(float frameTime) final override;
};

class ClipNode : public BlendNode
{
public:
	ClipNode(const gef::SkeletonPose& bindPose);
	bool ProcessData(float frameTime) final override;
	void SetPlaybackSpeed(float speed);
	void SetLooping(bool loop);
	void SetClip(const gef::Animation* clip);
	float GetPlaybackSpeed();
	bool IsLooping();
	const gef::Animation* GetClip();

private:
	float m_AnimationTime;
	float m_ClipPlaybackSpeed;
	bool m_ClipLooping;
	const gef::Animation* p_Clip;
};

class LinearBlendNode : public BlendNode
{
public:
	LinearBlendNode(const gef::SkeletonPose& bindPose);
	bool ProcessData(float frameTime) override;
	void SetBlendValuePtr(float* pBlendVal);

protected:
	float* p_BlendValue;
};

class LinearBlendNodeSync : public LinearBlendNode
{
public:
	LinearBlendNodeSync(const gef::SkeletonPose& bindPose);
	bool ProcessData(float frameTime) final override;

private:

};

enum class NodeType_
{
	NodeType_Undefined = -1,
	NodeType_Output,
	NodeType_Clip,
	NodeType_LinearBlend
};

class BlendTree
{
public:
	BlendTree(const gef::SkeletonPose& bindPose);
	~BlendTree();
	const gef::SkeletonPose& GetOutputPose();

	uint32_t AddNode(BlendNode* node);
	uint32_t AddNode(NodeType_ type);
	BlendNode* GetNode(uint32_t ID);
	void ConnectToRoot(uint32_t inputNodeID);
	void ConnectNode(uint32_t inputNodeID, uint32_t inputSlot, uint32_t receiverNodeID);
	void ConnectNodes(uint32_t inputNodeID1, uint32_t inputNodeID2, uint32_t receiverNodeID);
	void ConnectNodes(uint32_t inputNodeID1, uint32_t inputNodeID2, uint32_t inputNodeID3, uint32_t inputNodeID4, uint32_t receiverNodeID);

	void Update(float frameTime);

private:
	const gef::SkeletonPose& m_BindPose;
	std::vector<BlendNode*> v_Tree;
};