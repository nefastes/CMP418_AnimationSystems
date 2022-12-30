#pragma once
#include <stdint.h>
#include <array>
#include "animation/skeleton.h"
#include "Animation.h"

// Reserve space for up to 1000 nodes. It seems extreme to add more nodes than this.
#define BLENDTREE_MAXNODES 1000

enum class NodeType_
{
	NodeType_Undefined = -1,
	NodeType_Output,
	NodeType_Clip,
	NodeType_LinearBlend,
	NodeType_LinearBlendSync,
	NodeType_Ragdoll
};

class BlendNode
{
public:
	BlendNode(const gef::SkeletonPose& bindPose);
	void SetInput(uint32_t slot, BlendNode* input);
	void SetInput(BlendNode* input1, BlendNode* input2);
	void SetInput(BlendNode* input1, BlendNode* input2, BlendNode* input3, BlendNode* input4);
	void AddInput(BlendNode* input);
	void RemoveInput(BlendNode* input);
	bool Update(float frameTime);
	virtual bool ProcessData(float frameTime) = 0;
	const gef::SkeletonPose& GetPose();
	const NodeType_& GetType() const { return m_Type; }
	const std::array<BlendNode*, 4>& GetInputs() const { return a_Inputs; }

protected:
	std::array<BlendNode*, 4> a_Inputs; // Shouldnt need more than 4 inputs
	const gef::SkeletonPose& r_BindPose;
	gef::SkeletonPose m_BlendedPose;
	NodeType_ m_Type;
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
	void SetClip(const AsdfAnim::Clip* clip);
	float GetPlaybackSpeed();
	bool IsLooping();
	const AsdfAnim::Clip* GetClip();

private:
	float m_AnimationTime;
	float m_ClipPlaybackSpeed;
	bool m_ClipLooping;
	const AsdfAnim::Clip* p_Clip;
};

class LinearBlendNode : public BlendNode
{
public:
	LinearBlendNode(const gef::SkeletonPose& bindPose);
	bool ProcessData(float frameTime) override;
	void SetBlendValue(float pBlendVal);
	float* GetBlendValue();

protected:
	float m_BlendValue;
};

class LinearBlendNodeSync : public LinearBlendNode
{
public:
	LinearBlendNodeSync(const gef::SkeletonPose& bindPose);
	bool ProcessData(float frameTime) final override;

private:

};

class BlendTree
{
public:
	BlendTree(const gef::SkeletonPose& bindPose);
	~BlendTree();
	const gef::SkeletonPose& GetOutputPose();

	uint32_t AddNode(BlendNode* node);
	uint32_t AddNode(NodeType_ type);
	void RemoveAndFreeNode(BlendNode* node);
	BlendNode* GetNode(uint32_t ID);
	void ConnectToRoot(uint32_t inputNodeID);
	void ConnectNode(uint32_t inputNodeID, uint32_t inputSlot, uint32_t receiverNodeID);
	void ConnectNodes(uint32_t inputNodeID1, uint32_t inputNodeID2, uint32_t receiverNodeID);
	void ConnectNodes(uint32_t inputNodeID1, uint32_t inputNodeID2, uint32_t inputNodeID3, uint32_t inputNodeID4, uint32_t receiverNodeID);

	void Update(float frameTime);

	const std::vector<BlendNode*>& GetTree() const { return v_Tree; }
	const gef::SkeletonPose& GetBindPose() const { return m_BindPose; }

private:
	const gef::SkeletonPose& m_BindPose;
	std::vector<BlendNode*> v_Tree;
};