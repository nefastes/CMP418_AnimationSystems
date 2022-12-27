#pragma once
#include <string>
#include <vector>
#include <unordered_map>
#include "rapidjson/document.h"
#include "maths/vector2.h"
#include "maths/matrix33.h"
#include "graphics/sprite.h"
#include "maths/math_utils.h"	// TODO: Make your own and get rid of everything about GEF so this can be standalone
#include "animation.h"

#define DEG_TO_RAD(x) (3.1415f * (x) / 180.f)

namespace gef
{
	class Platform;
	class AnimatedSprite;
	class SpriteRenderer;
}

namespace AsdfAnim
{
	struct TransformData
	{
		float spriteWidth;
		float spriteHeight;
		float uvWidth;
		float uvHeight;
		gef::Vector2 uvPosition;
		gef::Matrix33 transform;
	};

	struct SpriteSheet
	{
		struct SubTexture
		{
			float frameY;
			float y;
			float frameWidth;
			float frameX;
			float frameHeight;
			float width;
			float height;
			std::string name;
			float x;
			gef::Matrix33 subTextureTransform;
		};

		float width;
		std::string path;
		float height;
		std::string name;
		std::unordered_map<std::string, SubTexture > subTexture;
	};

	struct Skeleton
	{
		float frameRate;
		std::string name;
		std::string version;
		std::string compatibleVersion;
		struct Armature
		{
			std::string type;
			float frameRate;
			std::string name;

			struct AABB {
				float x;
				float y;
				float width;
				float height;
			} aabb;								// Collisions - AABB bounding box

			struct Bone
			{
				float length;
				std::string name;
				std::string parent;
				struct Transform {
					float x;
					float y;
					float skX;
					float skY;
				} transform;			// Bone transform
			};
			std::unordered_map<std::string, Bone> bone;

			struct Slot {
				unsigned displayIndex;
				std::string name;
				std::string parent;
			};
			std::vector<Slot> slot;		// Associates a skin_slot name with a parent bone

			struct Skin
			{
				struct Slot {
					std::string name;
					struct Display {
						std::string name;
						struct Transform
						{
							float x;
							float y;
							float skX;
							float skY;
						} transform;			// Sprite offset transform
					};
					std::vector<Display> display;
				};
				std::unordered_map<std::string, Slot> slot;
			};
			std::vector<Skin> skin;

			struct Anim
			{
				unsigned duration;
				unsigned playTimes;
				std::string name;

				struct Slot {
					std::string name;
					struct DisplayFrame {
						unsigned value;
					};
					std::vector<DisplayFrame> displayFrame;
				};
				std::vector<Slot> slot;

				struct Bone {
					std::string name;

					struct TranslateFrame {							// Frame transform translation
						float startTime;
						float duration;
						unsigned tweenEasing;
						float x;
						float y;
					};
					std::vector<TranslateFrame> translateFrame;

					struct RotateFrame {							// Frame transform rotation
						float startTime;
						float duration;
						unsigned tweenEasing;
						float rotate;
					};
					std::vector<RotateFrame> rotateFrame;
				};
				std::unordered_map<std::string, Bone> bone;
			};
			std::unordered_map<std::string, Anim> animation;

			struct DefaultActions
			{
				std::string gotoAndPlay;
			};
			std::vector<DefaultActions> defaultActions;
		};
		std::vector<Armature> armature;
	};

	class Animation2D : public Animation
	{
	public:
		Animation2D();
		~Animation2D();
		static Animation2D* CreateFromJSON(gef::Platform& platform, const char* textureFilename, const char* skeletonFilename);
		static Animation2D* CreateFromJSON(gef::Platform& platform, const char* commonFilename);

		void Update(float dt) override;
		void Render(gef::SpriteRenderer* renderer2d);

		const char* GetSpriteSheetName();
		const unsigned& GetCurrentFrame();
		const char* GetCurrentFrameName();
		void CalculateTransformData();
		const TransformData& GetTransformData(unsigned index);
		const std::vector<TransformData>& GetAllTransforms();
		bool IsRigged();

		void PreviousFrame();
		void NextFrame();
		void TogglePlay();
		void SelectAnimation(const std::string& animationName);

		uint32_t GetArmatureCount() const { return m_Skeleton.armature.size(); }
		uint32_t GetCurrentArmature() const { return m_CurrentArmature; }
		const std::string& GetArmatureName(uint32_t index) const { return m_Skeleton.armature[index].name; }
		void SelectArmature(uint32_t s);

		std::vector<std::string> AvailableClips();

		void SetSprite(gef::AnimatedSprite* s);
		gef::AnimatedSprite* GetSprite();

		const std::string& GetFileName();

	private:
		void ReadSpriteSheetFromJSON(const rapidjson::Document& doc);
		void ReadSkeletonFromJSON(const rapidjson::Document& doc);
		const gef::Matrix33 BuildRigWorldTransform(const std::string& startBoneName);

	private:
		bool m_Playing;
		bool m_IsRigged;
		float m_Clock;
		uint32_t m_CurrentFrame;
		uint32_t m_CurrentArmature;
		SpriteSheet m_SpriteSheet;
		Skeleton m_Skeleton;
		std::vector<TransformData> m_TransformData;
		std::string m_CurrentAnimation;

		gef::AnimatedSprite* p_Sprite;
	};
}