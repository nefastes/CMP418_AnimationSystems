#pragma once
#include "motion_clip_player.h"
#include "Animation.h"
#include "BlendNode.h"
#include <vector>
#include <array>
#include "ragdoll.h"
#include "graphics/skinned_mesh_instance.h"

namespace gef
{
	class Platform;
	class Scene;
	class Mesh;
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
		const Clip* GetClip(const size_t animIndex) const { return &v_Clips[animIndex]; }
		const gef::Matrix44& GetMeshTransform() const { return p_MeshInstance->transform(); }
		void SetMeshTransform(const gef::Matrix44& transform) { p_MeshInstance->set_transform(transform); }
		const std::string& GetFileName() const { return s_Filename; }

		Ragdoll* GetRagdoll() const { return p_Ragdoll; }
		bool RequirePhysics() const { return m_NeedsPhysicsUpdate; }

		BlendTree* GetBlendTree() const { return p_BlendTree; }

	private:
		gef::Scene* p_Scene;
		gef::Mesh* p_Mesh;
		gef::SkinnedMeshInstance* p_MeshInstance;
		std::vector<Clip> v_Clips;
		std::vector<std::string> v_AvailableClips;
		Clip* p_CurrentAnimation;
		std::string s_Filename;

		// Physics
		Ragdoll* p_Ragdoll;
		bool m_NeedsPhysicsUpdate;

		// BlendTrees
		BlendTree* p_BlendTree;

	};

}