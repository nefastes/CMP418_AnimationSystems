#pragma once
#include <stdint.h>
#include <array>
#include "animation/skeleton.h"
#include "Animation.h"
#include "ragdoll.h"

// Reserve space for up to 1000 nodes. It seems extreme to add more nodes than this.
#define BLENDTREE_MAXNODES 1000

namespace AsdfAnim
{

	enum class NodeType_
	{
		NodeType_Undefined = -1,
		NodeType_Output,
		NodeType_Clip,
		NodeType_LinearBlend,
		NodeType_LinearBlendSync,
		NodeType_Transition,
		NodeType_Ragdoll
	};

	enum class TransitionType_
	{
		TransitionType_Undefined = -1,
		TransitionType_Frozen,
		TransitionType_Frozen_Sync,
		TransitionType_Smooth,
		TransitionType_Smooth_Sync
	};

	class BlendNode
	{
	public:
		BlendNode(const gef::SkeletonPose& bindPose);

		virtual bool SetInput(uint32_t slot, BlendNode* input) { a_Inputs[slot] = input; return true; }
		void SetInput(BlendNode* input1, BlendNode* input2) { a_Inputs = { input1, input2, nullptr, nullptr }; }
		void SetInput(BlendNode* input1, BlendNode* input2, BlendNode* input3, BlendNode* input4) { a_Inputs = { input1, input2, input3, input4 }; }
		void AddInput(BlendNode* input);
		void RemoveInput(BlendNode* input);

		virtual bool Update(float frameTime, bool& needPhysicsUpdate);
		virtual bool ProcessData(float frameTime) = 0;

		const gef::SkeletonPose& GetPose() const { return m_BlendedPose; }
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

		void SetPlaybackSpeed(float speed) { m_ClipPlaybackSpeed = speed; }
		void SetLooping(bool loop) { m_ClipLooping = loop; }
		void SetClip(const AsdfAnim::Clip* clip) { p_Clip = clip; }
		void SetAnimationTime(float time) { m_AnimationTime = time; }

		float GetPlaybackSpeed() const { return m_ClipPlaybackSpeed; }
		bool IsLooping() const { return m_ClipLooping; }
		const AsdfAnim::Clip* GetClip() const { return p_Clip; }
		float GetAnimationTime() { return m_AnimationTime; }

		void Reset() { m_AnimationTime = 0.f; m_ClipPlaybackSpeed = 1.f; m_ClipLooping = true; }

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

		void SetBlendValue(float pBlendVal) { m_BlendValue = pBlendVal; }
		float* GetBlendValuePtr() { return &m_BlendValue; }

	protected:
		float m_BlendValue;
	};

	class LinearBlendNodeSync : public LinearBlendNode
	{
	public:
		LinearBlendNodeSync(const gef::SkeletonPose& bindPose);
		bool ProcessData(float frameTime) override;
		bool SetInput(uint32_t slot, BlendNode* input) override;
		void CalculateClipsMaxMin();
		void CalculateBlendedSyncPose();
		void AssignNewClipSpeeds(ClipNode* input1, ClipNode* input2);

	protected:
		std::array<float, 2> a_ClipsMaxMin;
		std::array<uint32_t, 2> a_ClipIDs;
	};

	class TransitionNode : public LinearBlendNodeSync
	{
	public:
		TransitionNode(const gef::SkeletonPose& bindPose);
		bool Update(float frameTime, bool& needPhysicsUpdate) final override;
		bool ProcessData(float frameTime) final override;
		bool SetInput(uint32_t slot, BlendNode* input) final override;

		void StartTransition();
		void Reset();

		void SetTransitionType(const TransitionType_& type);
		const TransitionType_& GetTransitionType() const { return m_TransitionType; }
		bool IsTransitioning() const { return m_Transitioning; }
		void SetTransitionTime(float transitionTime) { m_TransitionTime = transitionTime; }
		float GetTransitionTime() const { return m_TransitionTime; }

	private:
		TransitionType_ m_TransitionType;
		bool m_Transitioning;
		float m_TransitionTime, m_CurrentTime;
	};

	class RagdollNode : public BlendNode
	{
	public:
		RagdollNode(const gef::SkeletonPose& bindPose);
		bool Update(float frameTime, bool& needPhysicsUpdate) final override;
		bool ProcessData(float frameTime) final override;

		void SetActive(bool a) { m_Active = a; }
		bool IsActive() const { return m_Active; }

		void SetRagdoll(Ragdoll* pRagdoll) { p_Ragdoll = pRagdoll; }
		bool IsRagdollValid() { return p_Ragdoll != nullptr; }

	private:
		Ragdoll* p_Ragdoll;
		bool m_Active;
	};

	class BlendTree
	{
	public:
		BlendTree(const gef::SkeletonPose& bindPose);
		~BlendTree();

		// Return the pose from the output node
		const gef::SkeletonPose& GetOutputPose() const { return v_Tree.front()->GetPose(); }

		uint32_t AddNode(BlendNode* node);
		uint32_t AddNode(NodeType_ type);
		void RemoveAndFreeNode(BlendNode* node);

		BlendNode* GetNode(uint32_t ID);
		void ConnectToRoot(uint32_t inputNodeID);
		// Connecting nodes is done on each particular node. See UserInterface.cpp

		void Update(float frameTime, bool& needsPhysicsUpdate);

		const std::vector<BlendNode*>& GetTree() const { return v_Tree; }
		const gef::SkeletonPose& GetBindPose() const { return m_BindPose; }

	private:
		const gef::SkeletonPose& m_BindPose;
		std::vector<BlendNode*> v_Tree;
	};

}