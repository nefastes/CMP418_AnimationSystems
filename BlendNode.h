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
		virtual bool SetInput(uint32_t slot, BlendNode* input);
		void SetInput(BlendNode* input1, BlendNode* input2);
		void SetInput(BlendNode* input1, BlendNode* input2, BlendNode* input3, BlendNode* input4);
		void AddInput(BlendNode* input);
		void RemoveInput(BlendNode* input);
		virtual bool Update(float frameTime, bool& needPhysicsUpdate);
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
		const AsdfAnim::Clip* GetClip() const;
		void SetAnimationTime(float time) { m_AnimationTime = time; }
		float GetAnimationTime() { return m_AnimationTime; }
		void ResetAnimationTime();

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
		bool ProcessData(float frameTime) override;
		bool SetInput(uint32_t slot, BlendNode* input) override;
		void CalculateClipsMaxMin();
		void CalculateBlendedSyncPose();

	protected:
		std::array<float, 2> a_ClipsMaxMin;
		std::array<AsdfAnim::ClipType, 2> a_ClipTypes;
	};

	class TransitionNode : public LinearBlendNodeSync
	{
	public:
		TransitionNode(const gef::SkeletonPose& bindPose);
		bool Update(float frameTime, bool& needPhysicsUpdate) final override;
		bool ProcessData(float frameTime) final override;
		bool SetInput(uint32_t slot, BlendNode* input) final override;
		void SetTransitionType(const TransitionType_& type) { m_TransitionType = type; }
		const TransitionType_& GetTransitionType() { return m_TransitionType; }
		void StartTransition();
		bool IsTransitioning() { return m_Transitioning; }
		void SetTransitionTime(float transitionTime) { m_TransitionTime = transitionTime; }
		float GetTransitionTime() { return m_TransitionTime; }
		void Reset();

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
		void SetRagdoll(Ragdoll* pRagdoll);

		bool IsActive() { return m_Active; }
		void SetActive(bool a) { m_Active = a; }

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
		const gef::SkeletonPose& GetOutputPose();

		uint32_t AddNode(BlendNode* node);
		uint32_t AddNode(NodeType_ type);
		void RemoveAndFreeNode(BlendNode* node);
		BlendNode* GetNode(uint32_t ID);
		void ConnectToRoot(uint32_t inputNodeID);
		void ConnectNode(uint32_t inputNodeID, uint32_t inputSlot, uint32_t receiverNodeID);
		void ConnectNodes(uint32_t inputNodeID1, uint32_t inputNodeID2, uint32_t receiverNodeID);
		void ConnectNodes(uint32_t inputNodeID1, uint32_t inputNodeID2, uint32_t inputNodeID3, uint32_t inputNodeID4, uint32_t receiverNodeID);

		void Update(float frameTime, bool& needsPhysicsUpdate);

		const std::vector<BlendNode*>& GetTree() const { return v_Tree; }
		const gef::SkeletonPose& GetBindPose() const { return m_BindPose; }

	private:
		const gef::SkeletonPose& m_BindPose;
		std::vector<BlendNode*> v_Tree;
	};

}