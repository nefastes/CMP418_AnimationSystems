#pragma once
#include <vector>
#include <string>
#include <tuple>

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

		void Update(float frameTime);
		void Draw2D(gef::SpriteRenderer* pRenderer2D);
		void Draw3D(gef::Renderer3D* pRenderer3D);

		void LoadGef3D(const char* filename);
		void LoadAllGef3DFromFolder(const char* folderpath, bool recursiveSearch = false);

		void LoadDragronbone2D(const char* filename);
		void LoadAllDragonbone2DFromFolder(const char* folderpath, bool recursiveSearch = false);

		void LoadDragronbone2DJson(const char* filename);
		void LoadAllDragonbone2DJsonFromFolder(const char* folderpath, bool recursiveSearch = false);

		const std::vector<const std::string*>& GetAvailableFileNames();
		const std::vector<Animation2D*>& GetAvailable2DDatas();
		const std::vector<Animation3D*>& GetAvailable3DDatas();

	private:
		gef::Platform& r_Platform;
		std::vector<Animation2D*>				v_LoadedAnimations2D;
		std::vector<Animation3D*>				v_LoadedAnimations3D;
		std::vector<const std::string*>			v_AvailableFiles;		// Storing the name as a string so it can be listed in the gui
	};

}