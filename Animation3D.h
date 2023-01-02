#pragma once
#include "motion_clip_player.h"
#include "Animation.h"
#include "BlendNode.h"
#include <vector>
#include <array>
#include "ragdoll.h"

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
		static Animation3D* CreateFromSceneFile(gef::Platform& platform, const char* folderpath);

		void LoadScene(gef::Platform& platform, const char* filepath);
		void LoadRagdoll(btDiscreteDynamicsWorld* pbtDynamicWorld, const char* filepath);

		virtual void Update(float frameTime) final override;
		void Draw(gef::Renderer3D* renderer) const;

		const std::vector<std::string>& AvailableClips() const { return v_AvailableClips; }
		const Clip* GetClip(const size_t animIndex) const;
		const gef::Matrix44& GetMeshTransform();
		void SetMeshTransform(const gef::Matrix44& transform);
		const std::string& GetFileName();

		void SetBodyVelocity(float v) { m_BodyVelociy = v; }
		const float& GetBodyVelocity() { return m_BodyVelociy; }

		BlendTree* GetBlendTree() const { return p_BlendTree; }

		Ragdoll* GetRagdoll() const { return p_Ragdoll; }

		bool RequirePhysics() const { return m_NeedsPhysicsUpdate; }

	private:
		gef::Scene* p_Scene;
		gef::Mesh* p_Mesh;
		gef::SkinnedMeshInstance* p_MeshInstance;
		std::vector<Clip> v_Clips;
		std::vector<std::string> v_AvailableClips;
		Clip* p_CurrentAnimation;
		std::string s_Filename;

		float m_BodyVelociy;

		BlendTree* p_BlendTree;

		Ragdoll* p_Ragdoll;
		bool m_NeedsPhysicsUpdate;
	};

}