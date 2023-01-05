#pragma once
#include <vector>
#include <string>
#include <tuple>

class btDiscreteDynamicsWorld;

namespace gef
{
	class Platform;
	class SpriteRenderer;
	class Renderer3D;
	class AnimatedSprite;
}

namespace AsdfAnim
{
	class Animation;
	class Animation3D;
	class Animation2D;

	class AnimationManager
	{
	public:
		AnimationManager(gef::Platform& rPlatform);
		~AnimationManager();
		void SetBtPhysicsWorld(btDiscreteDynamicsWorld* pbtDynamicWorld) { p_btDynamicWorld = pbtDynamicWorld; }

		void Update(float frameTime);
		void Draw2D(gef::SpriteRenderer* pRenderer2D) const;
		void Draw3D(gef::Renderer3D* pRenderer3D) const;

		void LoadGef3D(const char* filename);
		void LoadAllGef3DFromFolder(const char* folderpath, bool recursiveSearch = false);

		void LoadDragronbone2DJson(const char* filename);
		void LoadAllDragonbone2DJsonFromFolder(const char* folderpath, bool recursiveSearch = false);

		const std::vector<const std::string*>& GetAvailableFileNames() const { return v_AvailableFiles; } // This function should be called by the GUI to list all the available animators
		const std::vector<Animation2D*>& GetAvailable2DDatas() const { return v_LoadedAnimations2D; }
		const std::vector<Animation3D*>& GetAvailable3DDatas() const { return v_LoadedAnimations3D; }
		bool RequirePhysics() const { return m_NeedsPhysicsUpdate; }

	private:
		gef::Platform&							r_Platform;
		std::vector<Animation2D*>				v_LoadedAnimations2D;
		std::vector<Animation3D*>				v_LoadedAnimations3D;
		std::vector<const std::string*>			v_AvailableFiles;		// Storing the name as a string so it can be listed in the gui
		btDiscreteDynamicsWorld*				p_btDynamicWorld;		// A pointer to any physics world that exist. Must be set to load ragdolls
		bool									m_NeedsPhysicsUpdate;	// A bool that will be set to true if any animation requires a physics update
	};

}