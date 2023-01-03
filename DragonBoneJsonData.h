#pragma once
// This file defines the structures needed to load all the data from DragonBone JSON _tex & _ske files

namespace AsdfAnim
{
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
}