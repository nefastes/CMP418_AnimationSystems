#pragma once
#include "motion_clip_player.h"
#include "Animation.h"
#include "BlendNode.h"
#include <vector>
#include <array>

// Values from average speeds in m/s since 1u = 1m
#define PHYSICS_WALK_SPEED 1.38889f
#define PHYSICS_RUN_SPEED 3.57631944444f

namespace gef
{
	class Platform;
	class Scene;
	class Mesh;
	class SkinnedMeshInstance;
	class Animation;
	class Renderer3D;
	class Matrix44;
}

namespace AsdfAnim
{
	class Animation3D : public Animation
	{
	public:
		Animation3D();
		~Animation3D();
		static Animation3D* CreateFromFolder(gef::Platform& platform, const char* folderpath);
		static Animation3D* CreateFromSceneFile(gef::Platform& platform, const char* folderpath);

		void LoadScene(gef::Platform& platform, const char* filepath);

		virtual void Update(float frameTime) final override;
		void Draw(gef::Renderer3D* renderer) const;

		const std::vector<std::string>& AvailableClips() const { return v_AvailableClips; }
		void TransitionToAnimation(const size_t animIndex, float transitionTime, TransitionType transitionType);
		const Clip* GetClip(const size_t animIndex) const;
		const gef::Matrix44& GetMeshTransform();
		void SetMeshTransform(const gef::Matrix44& transform);
		const std::string& GetFileName();

		void SetBodyVelocity(float v) { m_BodyVelociy = v; }
		const float& GetBodyVelocity() { return m_BodyVelociy; }

		void InitBlendTree();
		BlendTree* GetBlendTree() const { return p_BlendTree; }

	private:
		gef::Scene* p_Scene;
		gef::Mesh* p_Mesh;
		gef::SkinnedMeshInstance* p_MeshInstance;
		std::vector<Clip> v_Clips;
		std::vector<std::string> v_AvailableClips;
		Clip* p_CurrentAnimation;
		std::string s_Filename;

		std::array<MotionClipPlayer, 2> m_AnimationPlayers;
		MotionClipPlayer* m_Activeplayer;
		gef::SkeletonPose m_BlendedPose;
		struct Transition {
			bool blend;
			float blendClock;
			float blendEndTime;
			TransitionType blendType;
			uint32_t blendClipPlayerIndex;
		} m_BlendInfo;

		float m_BodyVelociy;

		BlendTree* p_BlendTree;
	};

}