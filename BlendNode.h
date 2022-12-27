#pragma once
#include <stdint.h>
#include <array>
#include "animation/skeleton.h"
class BlendNode
{
public:
	BlendNode(const gef::SkeletonPose& bindPose);
	template<typename...B> void SetInputs(B*...args);
	bool Update(float frameTime);
	virtual bool ProcessData(float frameTime) = 0;
	const gef::SkeletonPose& GetPose();

protected:
	std::array<BlendNode*, 4> a_Inputs; // Shouldnt need more than 4 inputs
	const gef::SkeletonPose& p_BindPose;
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
	bool ProcessData(float frameTime) final override;

private:
	float m_BlendValue;
};

class BlendTree
{
public:
	BlendTree(const gef::SkeletonPose& bindPose);
	~BlendTree();
	const gef::SkeletonPose& GetOutputPose();

	uint32_t AddNode(BlendNode* node);
	BlendNode* GetNode(uint32_t index);

	void Update(float frameTime);

private:
	const gef::SkeletonPose& m_BindPose;
	std::vector<BlendNode*> m_Tree;
};