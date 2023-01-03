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
#include "DragonBoneJsonData.h"

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

	class Animation2D : public Animation
	{
	public:
		Animation2D();
		~Animation2D();
		static Animation2D* CreateFromJSON(gef::Platform& platform, const char* textureFilename, const char* skeletonFilename);
		static Animation2D* CreateFromJSON(gef::Platform& platform, const char* commonFilename);

		void Update(float dt) final override;
		void Render(gef::SpriteRenderer* renderer2d);

		const char* GetSpriteSheetName() const { return m_SpriteSheet.path.c_str(); }
		const unsigned& GetCurrentFrame() const { return m_CurrentFrame; }
		const char* GetCurrentFrameName();
		void CalculateTransformData();
		const TransformData& GetTransformData(unsigned index) const { return m_TransformData[index]; }
		const std::vector<TransformData>& GetAllTransforms() const { return m_TransformData; }
		bool IsRigged() const { return m_IsRigged; }

		void PreviousFrame();
		void NextFrame();
		void TogglePlay() { m_Playing = !m_Playing; }
		void SelectAnimation(const std::string& animationName);

		uint32_t GetArmatureCount() const { return m_Skeleton.armature.size(); }
		uint32_t GetCurrentArmature() const { return m_CurrentArmature; }
		const std::string& GetArmatureName(uint32_t index) const { return m_Skeleton.armature[index].name; }
		void SelectArmature(uint32_t s);

		std::vector<std::string> AvailableClips(); // Bad

		void SetSprite(gef::AnimatedSprite* s) { p_Sprite = s; }
		gef::AnimatedSprite* GetSprite() const { return p_Sprite; }

		const std::string& GetFileName() const { return m_Skeleton.name; }

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

		// GEF Dependecies
		// + Math libraries
		gef::AnimatedSprite* p_Sprite;
	};
}