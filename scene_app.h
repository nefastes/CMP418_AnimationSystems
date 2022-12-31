#ifndef _SCENE_APP_H
#define _SCENE_APP_H

#include <system/application.h>
#include <maths/vector2.h>
#include <graphics/sprite.h>
#include <input/input_manager.h>
#include <vector>
#include <graphics/skinned_mesh_instance.h>

#include "../imgui/imgui.h"
#include "../imgui/imgui_impl_dx11.h"
#include "../imgui/imgui_impl_win32.h"
#include "UserInterface.h"

#include "motion_clip_player.h"
#include "AnimationManager.h"
#include "Animation.h"

#include "Physics.h"

// FRAMEWORK FORWARD DECLARATIONS
namespace gef
{
	class Platform;
	class SpriteRenderer;
	class Font;
	class InputManager;
}

class SceneApp : public gef::Application
{
public:
	SceneApp(gef::Platform& platform);
	void Init();
	void CleanUp();
	bool Update(float frame_time);
	void Render();
private:
	void InitFont();
	void CleanUpFont();
	void DrawHUD();
    
	gef::Renderer3D* renderer_3d_;
	gef::SpriteRenderer* sprite_renderer_;
	gef::Font* font_;
	gef::InputManager* input_manager_;

	float fps_;

	// Animation manager and UI variables
	AsdfAnim::AnimationManager animation_manager_;

	// User Interface
	std::vector<size_t> gui_selected_subanimation_;
	std::vector<float> gui_animation_transition_time_;
	std::vector<AsdfAnim::TransitionType> gui_animation_transition_type_;
	std::vector<ImVec4> gui_animation_translations_;
	std::vector<ImVec4> gui_animation_rotations_;
	std::vector<ImVec4> gui_animation_scales_;
	bool editor_opened_;
	UI_NodeEditor editor_;

	// Physics
	Physics physics_engine_;
};

#endif // _SCENE_APP_H
