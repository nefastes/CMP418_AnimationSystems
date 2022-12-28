#include "scene_app.h"
#include <system/platform.h>
#include <graphics/sprite_renderer.h>
#include <graphics/font.h>
#include <system/debug_log.h>
#include <graphics/renderer_3d.h>
#include <graphics/mesh.h>
#include <maths/math_utils.h>
#include <input/sony_controller_input_manager.h>
#include <graphics/sprite.h>

//#include "motion_clip_player.h"
//#include "AnimationManager.h"
//#include "Animation.h"
#include "animation/skeleton.h"
#include "animation/animation.h"
#include "Animation.h"
#include "Animation3D.h"
#include "Animation2D.h"
#include "AnimatedSprite.h"

SceneApp::SceneApp(gef::Platform& platform) :
	Application(platform),
	sprite_renderer_(NULL),
	input_manager_(NULL),
	font_(NULL),
	animation_manager_(platform),
	editor_opened_(false),
	editor_("Editor")
{
}

void SceneApp::Init()
{
	// Init GEF components
	sprite_renderer_ = gef::SpriteRenderer::Create(platform_);
	renderer_3d_ = gef::Renderer3D::Create(platform_);
	input_manager_ = gef::InputManager::Create(platform_);
	InitFont();

	// Setup view and projection matrices
	gef::Matrix44 projection_matrix;
	gef::Matrix44 view_matrix;
	projection_matrix = platform_.PerspectiveProjectionFov(gef::DegToRad(45.0f), (float)platform_.width() / (float)platform_.height(), .01f, 1000.f);
	view_matrix.LookAt(gef::Vector4(-1.0f, 1.0f, 4.0f), gef::Vector4(0.0f, 1.0f, 0.0f), gef::Vector4(0.0f, 1.0f, 0.0f));
	renderer_3d_->set_projection_matrix(projection_matrix);
	renderer_3d_->set_view_matrix(view_matrix);

	// Setup a light
	gef::PointLight default_point_light;
	default_point_light.set_colour(gef::Colour(0.7f, 0.7f, 1.0f, 1.0f));
	default_point_light.set_position(gef::Vector4(-300.0f, -500.0f, 100.0f));
	gef::Default3DShaderData& default_shader_data = renderer_3d_->default_shader_data();
	default_shader_data.set_ambient_light_colour(gef::Colour(0.5f, 0.5f, 0.5f, 1.0f));
	default_shader_data.AddPointLight(default_point_light);

	// Load example animations
	animation_manager_.LoadAllGef3DFromFolder("", true);			// This will load all 3D animations within the media folder
	animation_manager_.LoadAllDragonbone2DJsonFromFolder("", true);	// This will load all 2D animations within the media folder (DragonBone)

	// Setup gui varaibles
	size_t size3d = animation_manager_.GetAvailable3DDatas().size();
	size_t size2d = animation_manager_.GetAvailable2DDatas().size();
	gui_selected_subanimation_.resize(size3d + size2d);
	gui_animation_transition_time_.resize(size3d, 1.f);
	gui_animation_transition_type_.resize(size3d, AsdfAnim::TransitionType::Transition_Type_Frozen);
	gui_animation_translations_.resize(size3d);
	gui_animation_rotations_.resize(size3d);
	gui_animation_scales_.resize(size3d, ImVec4(1.f, 1.f, 1.f, 0.f));
	editor_.OnStart();
}

void SceneApp::CleanUp()
{
	editor_.OnStop();

	delete input_manager_;
	input_manager_ = NULL;

	CleanUpFont();

	delete sprite_renderer_;
	sprite_renderer_ = NULL;
}

bool SceneApp::Update(float frame_time)
{
	fps_ = 1.0f / frame_time;

	input_manager_->Update();
	animation_manager_.Update(frame_time);

	return true;
}

void SceneApp::Render()
{
	// draw meshes here
	renderer_3d_->Begin();
	animation_manager_.Draw3D(renderer_3d_);
	renderer_3d_->End();

	// setup the sprite renderer, but don't clear the frame buffer
	// draw 2D sprites here
	sprite_renderer_->Begin(false);
	animation_manager_.Draw2D(sprite_renderer_);
	DrawHUD();
	sprite_renderer_->End();
}

void SceneApp::InitFont()
{
	font_ = new gef::Font(platform_);
	font_->Load("comic_sans");
}

void SceneApp::CleanUpFont()
{
	delete font_;
	font_ = NULL;
}

void SceneApp::DrawHUD()
{
	if(font_)
	{
		// display frame rate
		font_->RenderText(sprite_renderer_, gef::Vector4(850.0f, 510.0f, -0.9f), 1.0f, 0xffffffff, gef::TJ_LEFT, "FPS: %.1f", fps_);
	}

	ImGui_ImplDX11_NewFrame();
	ImGui_ImplWin32_NewFrame();
	ImGui::NewFrame();
	ImGui::DockSpaceOverViewport(ImGui::GetMainViewport(), ImGuiDockNodeFlags_PassthruCentralNode);

	//ImGui::ShowDemoWindow();

	//static bool imgui = true;
	const std::vector<const std::string*>& availableNames = animation_manager_.GetAvailableFileNames();
	const std::vector<AsdfAnim::Animation2D*>& available2D = animation_manager_.GetAvailable2DDatas();
	const std::vector<AsdfAnim::Animation3D*>& available3D = animation_manager_.GetAvailable3DDatas();
	ImGui::Begin("Animation System"/*, &imgui, ImGuiWindowFlags_NoTitleBar*/);
	if (ImGui::BeginTabBar("tabs"))
	{
		for (uint32_t i = 0; i < availableNames.size(); ++i)
		{
			AsdfAnim::Animation* currentAnim = i >= available3D.size() ? (AsdfAnim::Animation*)available2D[i - available3D.size()] : (AsdfAnim::Animation*)available3D[i];
			bool active = currentAnim->IsActive();

			if (ImGui::BeginTabItem(availableNames[i]->c_str()))
			{
				if (ImGui::Checkbox("Active", &active))
					currentAnim->SetActive(active);

				ImGui::NewLine();
				ImGui::Separator();

				if (currentAnim->IsType(AsdfAnim::AnimationType::Animation_Type_2D) || currentAnim->IsType(AsdfAnim::AnimationType::Animation_Type_2D_Rigged))
				{
					AsdfAnim::Animation2D* current2D = reinterpret_cast<AsdfAnim::Animation2D*>(currentAnim);
					if (current2D->GetArmatureCount() > 1)
					{
						if (ImGui::BeginCombo("Armature", current2D->GetArmatureName(current2D->GetCurrentArmature()).c_str()))
						{
							for (uint32_t j = 0u; j < current2D->GetArmatureCount(); ++j)
							{
								const bool selected = gui_selected_subanimation_[i] == j;
								if (ImGui::Selectable(current2D->GetArmatureName(j).c_str(), selected))
									current2D->SelectArmature(j);

								if (selected)
									ImGui::SetItemDefaultFocus();
							}
							ImGui::EndCombo();
						}
					}

					const std::vector<std::string>& availableAnims = current2D->AvailableClips();
					if (ImGui::BeginCombo("Clip", availableAnims.at(gui_selected_subanimation_[i]).c_str()))
					{
						for (size_t j = 0u; j < availableAnims.size(); ++j)
						{
							const bool selected = gui_selected_subanimation_[i] == j;
							if (ImGui::Selectable(availableAnims[j].c_str(), selected))
							{
								gui_selected_subanimation_[i] = j;
								current2D->SelectAnimation(availableAnims[gui_selected_subanimation_[i]]);
							}

							if (selected)
								ImGui::SetItemDefaultFocus();
						}
						ImGui::EndCombo();
					}

					gef::Vector2 spritePos = current2D->GetSprite()->GetBodyPosition();
					if (ImGui::DragFloat2("Position", &spritePos.x), .1f)
						current2D->GetSprite()->SetBodyPosition(spritePos);

					float spriteRoatation = current2D->GetSprite()->GetBodyRotation();
					if (ImGui::DragFloat("Rotation", &spriteRoatation), 1.f, 0.f, 360.f)
						current2D->GetSprite()->SetBodyRotation(spriteRoatation);

					gef::Vector2 spriteScale = current2D->GetSprite()->GetBodyScale();
					if (ImGui::DragFloat2("Scale", &spriteScale.x), .1f)
						current2D->GetSprite()->SetBodyScale(spriteScale);
				}
				else if (currentAnim->IsType(AsdfAnim::AnimationType::Animation_Type_3D))
				{
					AsdfAnim::Animation3D* current3D = reinterpret_cast<AsdfAnim::Animation3D*>(currentAnim);

					// The transform of the mesh
					ImGui::Text("Translation XYZ");
					if (ImGui::DragFloat3("Translation XYZ", &gui_animation_translations_[i].x, .1f))
					{
						gef::Vector4 t(gui_animation_translations_[i].x, gui_animation_translations_[i].y, gui_animation_translations_[i].z);
						gef::Matrix44 transform = current3D->GetMeshTransform();
						transform.SetTranslation(t);
						current3D->SetMeshTransform(transform);
					}
					ImGui::Text("Rotation Pitch Yaw Roll");
					if (ImGui::DragFloat3("Rotation Pitch Yaw Roll", &gui_animation_rotations_[i].x, 1.f))
					{
						gef::Matrix44 pitch, yaw, roll, translation, scale;
						gef::Matrix44 transform = current3D->GetMeshTransform();
						const gef::Vector4 t(gui_animation_translations_[i].x, gui_animation_translations_[i].y, gui_animation_translations_[i].z);
						const gef::Vector4 s(gui_animation_scales_[i].x, gui_animation_scales_[i].y, gui_animation_scales_[i].z);
						pitch.RotationX(gef::DegToRad(gui_animation_rotations_[i].x));
						yaw.RotationY(gef::DegToRad(gui_animation_rotations_[i].y));
						roll.RotationZ(gef::DegToRad(gui_animation_rotations_[i].z));
						translation.SetIdentity();
						translation.SetTranslation(t);
						scale.Scale(s);
						transform = scale * pitch * yaw * roll * translation;
						current3D->SetMeshTransform(transform);
					}
					ImGui::Text("Scale XYZ");
					if (ImGui::DragFloat3("Scale XYZ", &gui_animation_scales_[i].x, .1f))
					{
						gef::Vector4 t(gui_animation_scales_[i].x, gui_animation_scales_[i].y, gui_animation_scales_[i].z);
						gef::Matrix44 transform = current3D->GetMeshTransform();
						transform.Scale(t);
						current3D->SetMeshTransform(transform);
					}
					ImGui::SameLine();
					if (ImGui::Button("Scale for x/y-bot"))
					{
						gui_animation_scales_[i] = ImVec4(.01f, .01f, .01f, 0.f);
						gef::Vector4 t(gui_animation_scales_[i].x, gui_animation_scales_[i].y, gui_animation_scales_[i].z);
						gef::Matrix44 transform = current3D->GetMeshTransform();
						transform.Scale(t);
						current3D->SetMeshTransform(transform);
					}

					ImGui::Text("Edit Animation");
					if (ImGui::Button("Open Animation Editor"))
					{
						editor_opened_ = true;
						ImGui::SetNextWindowFocus();
					}

					// Do this seperately, since the user can keep it opened and navigate tabs on this window. Changes should take effect automatically
					if(editor_opened_)
						editor_.ResetFor(current3D);
				}

				ImGui::EndTabItem();
			}
		}
		ImGui::EndTabBar();
	}
	ImGui::End();

	if (editor_opened_)
	{
		if (ImGui::Begin("Content", &editor_opened_, ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse | ImGuiWindowFlags_NoBringToFrontOnFocus))
		{
			//Application_Frame();
			editor_.OnFrame(NULL);
		}
		ImGui::End();
	}

	ImGui::Render();
	ImGui_ImplDX11_RenderDrawData(ImGui::GetDrawData());

	ImGuiIO& io = ImGui::GetIO();
	if (io.ConfigFlags & ImGuiConfigFlags_ViewportsEnable)
	{
		ImGui::UpdatePlatformWindows();
		ImGui::RenderPlatformWindowsDefault();
	}
}

