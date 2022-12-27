#include "Animation2D.h"
#include "gef_json_loader.h"
#include "AnimatedSprite.h"
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
AsdfAnim::Animation2D::Animation2D() : m_Playing(true), m_IsRigged(false), m_Clock(0.f), m_CurrentFrame(0u), m_CurrentArmature(0u), m_SpriteSheet{}, m_Skeleton{}, p_Sprite(nullptr)
{
}

AsdfAnim::Animation2D::~Animation2D()
{
	if (p_Sprite)
	{
		delete p_Sprite;
		p_Sprite = nullptr;
	}
}

AsdfAnim::Animation2D* AsdfAnim::Animation2D::CreateFromJSON(gef::Platform& platform, const char* commonFilename)
{
	std::string textureFilename(commonFilename), skeletonFilename(commonFilename);
	textureFilename.append("_tex.json");
	skeletonFilename.append("_ske.json");
	return CreateFromJSON(platform, textureFilename.c_str(), skeletonFilename.c_str());
}

AsdfAnim::Animation2D* AsdfAnim::Animation2D::CreateFromJSON(gef::Platform& platform, const char* textureFilename, const char* skeletonFilename)
{
	char* texjson, * skeljson;
	rapidjson::Document textureJSON, skeletonJSON;
	Animation2D* result = new Animation2D();
	std::string spriteSheetPath(skeletonFilename);

	// Load the JSON file data
	if (!textureFilename) goto FAILED;

	texjson = LoadJSON(textureFilename);
	if (!texjson) goto FAILED;
	else textureJSON.Parse(texjson);

	// Read the data from the JSON into the sprite sheet
	result->ReadSpriteSheetFromJSON(textureJSON);

	// Repeat the same process if skeleton data is available
	if (!skeletonFilename) goto FAILED;

	skeljson = LoadJSON(skeletonFilename);
	if (!skeljson) goto FAILED;
	else skeletonJSON.Parse(skeljson);

	result->ReadSkeletonFromJSON(skeletonJSON);

	// Construct a new sprite
	spriteSheetPath.erase(spriteSheetPath.begin() + spriteSheetPath.find_last_of('\\') + 1, spriteSheetPath.end());
	spriteSheetPath.append(result->GetSpriteSheetName());
	result->SetSprite(new gef::AnimatedSprite(platform, spriteSheetPath.c_str()));
	result->CalculateTransformData();
	result->SetType(result->IsRigged() ? AnimationType::Animation_Type_2D_Rigged : AnimationType::Animation_Type_2D);

	// Return the constructed Animation2D
	return result;

FAILED:
	MessageBox(NULL, L"Error: Could not load the specified JSON during the initialisation of an Animation2D object.", L"Error", NULL);
	exit(-1);
}

void AsdfAnim::Animation2D::Update(float dt)
{
	if (!m_IsRigged)
	{
		m_Clock += dt;
		if (m_Clock > 1.f / static_cast<float>(m_Skeleton.armature[m_CurrentArmature].frameRate))
		{
			// Update the current frame
			++m_CurrentFrame;
			if (m_CurrentFrame >= m_Skeleton.armature[m_CurrentArmature].animation[m_CurrentAnimation].duration) m_CurrentFrame = 0u;
			m_Clock = 0.f;

			// Recalculate transforms
			CalculateTransformData();
		}
	}
	else
	{
		// Recalculate transforms
		CalculateTransformData();

		// Reset clock
		const float anim_time_in_sec = 1.f / m_Skeleton.armature[m_CurrentArmature].frameRate * m_Skeleton.armature[m_CurrentArmature].animation[m_CurrentAnimation].duration;
		m_Clock = fmod(m_Clock + dt, anim_time_in_sec);
	}
}

void AsdfAnim::Animation2D::Render(gef::SpriteRenderer* renderer2d)
{
	if (m_IsRigged)
	{
		for (unsigned i = 0u; i < m_TransformData.size(); ++i)
		{
			p_Sprite->set_width(m_TransformData[i].spriteWidth);
			p_Sprite->set_height(m_TransformData[i].spriteHeight);
			p_Sprite->set_uv_width(m_TransformData[i].uvWidth);
			p_Sprite->set_uv_height(m_TransformData[i].uvHeight);
			p_Sprite->set_uv_position(m_TransformData[i].uvPosition);
			renderer2d->DrawSprite(*p_Sprite, m_TransformData[i].transform);
		}
	}
	else
	{
		
		const AsdfAnim::TransformData& data = m_TransformData[m_CurrentFrame];
		p_Sprite->set_width(data.spriteWidth);
		p_Sprite->set_height(data.spriteHeight);
		p_Sprite->set_uv_width(data.uvWidth);
		p_Sprite->set_uv_height(data.uvHeight);
		p_Sprite->set_uv_position(data.uvPosition);
		renderer2d->DrawSprite(*p_Sprite, data.transform);
	}
}

const char* AsdfAnim::Animation2D::GetSpriteSheetName()
{
	return m_SpriteSheet.path.c_str();
}

const unsigned& AsdfAnim::Animation2D::GetCurrentFrame()
{
	return m_CurrentFrame;
}

const char* AsdfAnim::Animation2D::GetCurrentFrameName()
{
	if (m_Skeleton.armature.size() == 1 && m_Skeleton.armature.back().type == "Sheet")
		return m_Skeleton.armature[m_CurrentArmature].skin[0].slot["sheetSlot"].display[m_Skeleton.armature[m_CurrentArmature].animation[m_CurrentAnimation].slot[0].displayFrame[m_CurrentFrame].value].name.c_str();
	else
		return m_Skeleton.armature[m_CurrentArmature].animation[m_CurrentAnimation].name.c_str();
}

void AsdfAnim::Animation2D::CalculateTransformData()
{
	// TODO: Refactor this
	const gef::Vector2& spriteBodyPos = p_Sprite->GetBodyPosition();
	const float& spriteBodyRotation = p_Sprite->GetBodyRotation();
	const gef::Vector2& spriteBodyScale = p_Sprite->GetBodyScale();
	if (!m_IsRigged)
	{
		const unsigned displayFrame = m_Skeleton.armature[m_CurrentArmature].animation[m_CurrentAnimation].slot[0].displayFrame[m_CurrentFrame].value;
		const std::string& frameName = m_Skeleton.armature[m_CurrentArmature].skin[0].slot["sheetSlot"].display[displayFrame].name;
		const SpriteSheet::SubTexture& subTexture = m_SpriteSheet.subTexture.at(frameName);	// TODO: StRiNg LoOkUp... I don't like this ):
		gef::Vector2 uv(subTexture.x / m_SpriteSheet.width, subTexture.y / m_SpriteSheet.height);
		gef::Vector2 spritePos(
			spriteBodyPos.x + (subTexture.width * .5f - (subTexture.frameWidth * .5f + subTexture.frameX)),
			spriteBodyPos.y + (subTexture.height * .5f - (subTexture.frameHeight * .5f + subTexture.frameY))
		);
		gef::Matrix33 temp, transform = temp = gef::Matrix33::kIdentity;
		transform.Scale(gef::Vector2(subTexture.width * spriteBodyScale.x, subTexture.height * spriteBodyScale.y));
		temp.Rotate(DEG_TO_RAD(spriteBodyRotation));
		transform = transform * temp;
		transform.SetTranslation(spritePos);

		m_TransformData[m_CurrentFrame].spriteWidth = subTexture.width;
		m_TransformData[m_CurrentFrame].spriteHeight = subTexture.height;
		m_TransformData[m_CurrentFrame].uvWidth = subTexture.width / m_SpriteSheet.width;
		m_TransformData[m_CurrentFrame].uvHeight = subTexture.height / m_SpriteSheet.height;
		m_TransformData[m_CurrentFrame].uvPosition = uv;
		m_TransformData[m_CurrentFrame].transform = transform;
	}
	else
	{
		// TODO: Replace hard-coded scale and stuff appropriately
		gef::Matrix33 temp, riggedTransform = temp = gef::Matrix33::kIdentity;
		riggedTransform.Scale(spriteBodyScale);
		temp.Rotate(DEG_TO_RAD(spriteBodyRotation));
		riggedTransform = riggedTransform * temp;
		riggedTransform.SetTranslation(spriteBodyPos);

		for (unsigned i = 0u; i < m_TransformData.size(); ++i)
		{
			const Skeleton::Armature::Slot& currentSlot = m_Skeleton.armature[m_CurrentArmature].slot[i];
			const Skeleton::Armature::Skin::Slot::Display& currentDisplay = m_Skeleton.armature[m_CurrentArmature].skin[0].slot[currentSlot.name].display[0];

			// Get STT
			const SpriteSheet::SubTexture& subTexture = m_SpriteSheet.subTexture.at(currentDisplay.name);
			gef::Vector2 uv(subTexture.x / m_SpriteSheet.width, subTexture.y / m_SpriteSheet.height);
			m_TransformData[i].spriteWidth = subTexture.width;
			m_TransformData[i].spriteHeight = subTexture.height;
			m_TransformData[i].uvWidth = subTexture.width / m_SpriteSheet.width;
			m_TransformData[i].uvHeight = subTexture.height / m_SpriteSheet.height;
			m_TransformData[i].uvPosition = uv;
			const gef::Matrix33& subTextureTransform = subTexture.subTextureTransform;

			// Get SOT
			gef::Matrix33 spriteOffsetTransform = gef::Matrix33::kIdentity;
			spriteOffsetTransform.Rotate(DEG_TO_RAD(currentDisplay.transform.skX));
			spriteOffsetTransform.SetTranslation(gef::Vector2(currentDisplay.transform.x, currentDisplay.transform.y));

			// Get WBT
			gef::Matrix33 worldBoneTransform = BuildRigWorldTransform(currentSlot.parent);

			// Build final matrix
			m_TransformData[i].transform = subTextureTransform * spriteOffsetTransform * worldBoneTransform * riggedTransform;
		}
	}
}

const std::vector<AsdfAnim::TransformData>& AsdfAnim::Animation2D::GetAllTransforms()
{
	return m_TransformData;
}

const AsdfAnim::TransformData& AsdfAnim::Animation2D::GetTransformData(unsigned index)
{
	return m_TransformData[index];
}

bool AsdfAnim::Animation2D::IsRigged()
{
	return m_IsRigged;
}

void AsdfAnim::Animation2D::PreviousFrame()
{
	--m_CurrentFrame;
	if (m_CurrentFrame > m_Skeleton.armature[m_CurrentArmature].animation[m_CurrentAnimation].duration) m_CurrentFrame = m_Skeleton.armature[m_CurrentArmature].animation[m_CurrentAnimation].duration - 1u;
	//UpdateSprite(sprite, spriteTargetPosition);
	CalculateTransformData();
}

void AsdfAnim::Animation2D::NextFrame()
{
	++m_CurrentFrame;
	if (m_CurrentFrame >= m_Skeleton.armature[m_CurrentArmature].animation[m_CurrentAnimation].duration) m_CurrentFrame = 0u;
	//UpdateSprite(sprite, spriteTargetPosition);
	CalculateTransformData();
}

void AsdfAnim::Animation2D::TogglePlay()
{
	m_Playing = !m_Playing;
}

void AsdfAnim::Animation2D::SelectAnimation(const std::string& Animation2DName)
{
	// Reset the clock
	m_Clock = 0.f;

	// Change the current Animation2D to be played
	m_CurrentAnimation = Animation2DName;
}

void AsdfAnim::Animation2D::SelectArmature(uint32_t s)
{
	assert(s < m_Skeleton.armature.size());
	m_CurrentArmature = s;
	
	m_CurrentAnimation = m_Skeleton.armature[m_CurrentArmature].animation.begin()->first;
}

std::vector<std::string> AsdfAnim::Animation2D::AvailableClips()
{
	std::vector<std::string> result;
	result.reserve(m_Skeleton.armature[m_CurrentArmature].animation.size());
	for (auto anim : m_Skeleton.armature[m_CurrentArmature].animation)
		result.push_back(anim.first);
	return result;
}

void AsdfAnim::Animation2D::SetSprite(gef::AnimatedSprite* s)
{
	p_Sprite = s;
}

gef::AnimatedSprite* AsdfAnim::Animation2D::GetSprite()
{
	return p_Sprite;
}

const std::string& AsdfAnim::Animation2D::GetFileName()
{
	return m_Skeleton.name;
}

void AsdfAnim::Animation2D::ReadSpriteSheetFromJSON(const rapidjson::Document& doc)
{
	// Fill in the sprite sheet with the JSON information
	if (doc.HasMember("name"))		m_SpriteSheet.name = doc["name"].GetString();
	if (doc.HasMember("imagePath"))	m_SpriteSheet.path = doc["imagePath"].GetString();
	if (doc.HasMember("width"))		m_SpriteSheet.width = doc["width"].GetFloat();
	if (doc.HasMember("height"))		m_SpriteSheet.height = doc["height"].GetFloat();

	// Load the subtextures
	const rapidjson::Value& subtextures = doc["SubTexture"];
	m_TransformData.resize(subtextures.Size());
	for (unsigned i = 0u; i < subtextures.Size(); ++i)
	{
		// Read the subtexture data
		SpriteSheet::SubTexture current = {};
		if (subtextures[i].HasMember("name"))		current.name = subtextures[i]["name"].GetString();
		if (subtextures[i].HasMember("x"))			current.x = subtextures[i]["x"].GetFloat();
		if (subtextures[i].HasMember("y"))			current.y = subtextures[i]["y"].GetFloat();
		if (subtextures[i].HasMember("width"))		current.width = subtextures[i]["width"].GetFloat();
		if (subtextures[i].HasMember("height"))		current.height = subtextures[i]["height"].GetFloat();
		if (subtextures[i].HasMember("frameX"))		current.frameX = subtextures[i]["frameX"].GetFloat();
		if (subtextures[i].HasMember("frameY"))		current.frameY = subtextures[i]["frameY"].GetFloat();
		if (subtextures[i].HasMember("frameWidth"))	current.frameWidth = subtextures[i]["frameWidth"].GetFloat();		else current.frameWidth = current.width;
		if (subtextures[i].HasMember("frameHeight"))current.frameHeight = subtextures[i]["frameHeight"].GetFloat();		else current.frameHeight = current.height;

		// Calculate the transform
		gef::Matrix33 scale = gef::Matrix33::kIdentity, translation = gef::Matrix33::kIdentity;
		scale.Scale(gef::Vector2(current.width, current.height));	// No need to set identity thanks gef
		translation.SetTranslation(gef::Vector2(
			current.width * .5f - (current.frameWidth * .5f + current.frameX),
			current.height * .5f - (current.frameHeight * .5f + current.frameY)
		));
		current.subTextureTransform = scale * translation;

		// Save it
		m_SpriteSheet.subTexture.insert({ current.name, current });
	}
}

void AsdfAnim::Animation2D::ReadSkeletonFromJSON(const rapidjson::Document& doc)
{
	// Fill in the skeleton with JSON data
	if (doc.HasMember("frameRate"))			m_Skeleton.frameRate = doc["frameRate"].GetFloat();
	if (doc.HasMember("name"))				m_Skeleton.name = doc["name"].GetString();
	if (doc.HasMember("version"))			m_Skeleton.version = doc["version"].GetString();
	if (doc.HasMember("compatibleVersion"))	m_Skeleton.compatibleVersion = doc["compatibleVersion"].GetString();

	const rapidjson::Value& armature = doc["armature"];
	for (unsigned i = 0u; i < armature.Size(); ++i)
	{
		// Armature
		Skeleton::Armature armature_current = {};
		if (armature[i].HasMember("type"))		armature_current.type = armature[i]["type"].GetString();
		if (armature[i].HasMember("frameRate"))	armature_current.frameRate = armature[i]["frameRate"].GetFloat();
		if (armature[i].HasMember("name"))		armature_current.name = armature[i]["name"].GetString();

		// AABB
		if (armature[i].HasMember("aabb"))
		{
			const rapidjson::Value& aabb = armature[i]["aabb"];
			if (aabb.HasMember("x"))			armature_current.aabb.x = aabb["x"].GetFloat();
			if (aabb.HasMember("y"))			armature_current.aabb.y = aabb["y"].GetFloat();
			if (aabb.HasMember("width"))		armature_current.aabb.width = aabb["width"].GetFloat();
			if (aabb.HasMember("height"))		armature_current.aabb.height = aabb["height"].GetFloat();
		}

		// Bone
		if (armature[i].HasMember("bone"))
		{
			const rapidjson::Value& bone = armature[i]["bone"];
			for (unsigned j = 0u; j < bone.Size(); ++j)
			{
				Skeleton::Armature::Bone bone_current = {};

				if (bone[j].HasMember("length"))	bone_current.length = bone[j]["length"].GetFloat();
				if (bone[j].HasMember("name"))		bone_current.name = bone[j]["name"].GetString();
				if (bone[j].HasMember("parent"))	bone_current.parent = bone[j]["parent"].GetString();
				if (bone[j].HasMember("transform"))
				{
					if (bone[j]["transform"].HasMember("x"))	bone_current.transform.x = bone[j]["transform"]["x"].GetFloat();
					if (bone[j]["transform"].HasMember("y"))	bone_current.transform.y = bone[j]["transform"]["y"].GetFloat();
					if (bone[j]["transform"].HasMember("skX"))	bone_current.transform.skX = bone[j]["transform"]["skX"].GetFloat();
					if (bone[j]["transform"].HasMember("skY"))	bone_current.transform.skY = bone[j]["transform"]["skY"].GetFloat();
				}

				armature_current.bone.insert({ bone_current.name, bone_current });
			}
		}

		// Slot
		if (armature[i].HasMember("slot"))
		{
			const rapidjson::Value& slot = armature[i]["slot"];
			for (unsigned j = 0u; j < slot.Size(); ++j)
			{
				Skeleton::Armature::Slot slot_current = {};

				if (slot[j].HasMember("displayIndex"))	slot_current.displayIndex = slot[j]["displayIndex"].GetUint();
				if (slot[j].HasMember("name"))			slot_current.name = slot[j]["name"].GetString();
				if (slot[j].HasMember("parent"))			slot_current.parent = slot[j]["parent"].GetString();

				armature_current.slot.push_back(slot_current);
			}
		}

		// Skin
		if (armature[i].HasMember("skin"))
		{
			const rapidjson::Value& skin = armature[i]["skin"];
			for (unsigned j = 0u; j < skin.Size(); ++j)
			{
				Skeleton::Armature::Skin skin_current = {};

				if (skin[j].HasMember("slot"))
				{
					const rapidjson::Value& skin_slot = skin[j]["slot"];
					for (unsigned k = 0u; k < skin_slot.Size(); ++k)
					{
						Skeleton::Armature::Skin::Slot skin_slot_current = {};

						if (skin_slot[k].HasMember("name")) skin_slot_current.name = skin_slot[k]["name"].GetString();
						if (skin_slot[k].HasMember("display"))
						{
							const rapidjson::Value& skin_slot_display = skin_slot[k]["display"];
							for (unsigned l = 0u; l < skin_slot_display.Size(); ++l)
							{
								Skeleton::Armature::Skin::Slot::Display skin_slot_display_current = {};

								if (skin_slot_display[l].HasMember("name"))
									skin_slot_display_current.name = skin_slot_display[l]["name"].GetString();
								if (skin_slot_display[l].HasMember("transform"))
								{
									skin_slot_display_current.transform = {
									skin_slot_display[l]["transform"].HasMember("x") ? skin_slot_display[l]["transform"]["x"].GetFloat() : 0.f,
									skin_slot_display[l]["transform"].HasMember("y") ? skin_slot_display[l]["transform"]["y"].GetFloat() : 0.f,
									skin_slot_display[l]["transform"].HasMember("skX") ? skin_slot_display[l]["transform"]["skX"].GetFloat() : 0.f,
									skin_slot_display[l]["transform"].HasMember("skY") ? skin_slot_display[l]["transform"]["skY"].GetFloat() : 0.f
									};
								}

								skin_slot_current.display.push_back(skin_slot_display_current);
							}
						}

						skin_current.slot.insert({ skin_slot_current.name, skin_slot_current });
					}
				}

				armature_current.skin.push_back(skin_current);
			}
		}

		// Animation2D
		if (armature[i].HasMember("animation"))
		{
			const rapidjson::Value& Animation2D = armature[i]["animation"];
			for (unsigned j = 0u; j < Animation2D.Size(); ++j)
			{
				Skeleton::Armature::Anim Animation2D_current = {};

				if (Animation2D[j].HasMember("duration"))	Animation2D_current.duration = Animation2D[j]["duration"].GetUint();
				if (Animation2D[j].HasMember("playTimes")) Animation2D_current.playTimes = Animation2D[j]["playTimes"].GetUint();
				if (Animation2D[j].HasMember("name"))		Animation2D_current.name = Animation2D[j]["name"].GetString();
				// Slot
				if (Animation2D[j].HasMember("slot"))
				{
					const rapidjson::Value& Animation2D_slot = Animation2D[j]["slot"];
					for (unsigned k = 0u; k < Animation2D_slot.Size(); ++k)
					{
						Skeleton::Armature::Anim::Slot Animation2D_slot_current = {};
						if (Animation2D_slot[k].HasMember("name"))
							Animation2D_slot_current.name = Animation2D_slot[k]["name"].GetString();
						if (Animation2D_slot[k].HasMember("displayFrame"))
						{
							const rapidjson::Value& displayFrame = Animation2D_slot[k]["displayFrame"];
							for (unsigned l = 0u; l < displayFrame.Size(); ++l)
							{
								Skeleton::Armature::Anim::Slot::DisplayFrame Animation2D_slot_displayFrame_current = {};

								if (displayFrame[l].HasMember("value"))
									Animation2D_slot_displayFrame_current.value = displayFrame[l]["value"].GetUint();

								Animation2D_slot_current.displayFrame.push_back(Animation2D_slot_displayFrame_current);
							}
						}

						Animation2D_current.slot.push_back(Animation2D_slot_current);
					}
				} // Slot
				// Bone
				if (Animation2D[j].HasMember("bone"))
				{
					const rapidjson::Value& Animation2D_bone = Animation2D[j]["bone"];
					for (unsigned l = 0u; l < Animation2D_bone.Size(); ++l)
					{
						Skeleton::Armature::Anim::Bone Animation2D_bone_current = {};

						if (Animation2D_bone[l].HasMember("name"))
							Animation2D_bone_current.name = Animation2D_bone[l]["name"].GetString();
						if (Animation2D_bone[l].HasMember("translateFrame"))
						{
							const rapidjson::Value& Animation2D_bone_translateFrame = Animation2D_bone[l]["translateFrame"];
							float translateFrameStartTime = 0.f;
							for (unsigned z = 0u; z < Animation2D_bone_translateFrame.Size(); ++z)
							{
								Skeleton::Armature::Anim::Bone::TranslateFrame Animation2D_bone_translateFrame_current = {};

								if (Animation2D_bone_translateFrame[z].HasMember("duration"))
									Animation2D_bone_translateFrame_current.duration = Animation2D_bone_translateFrame[z]["duration"].GetFloat();
								if (Animation2D_bone_translateFrame[z].HasMember("tweenEasing"))
									Animation2D_bone_translateFrame_current.tweenEasing = Animation2D_bone_translateFrame[z]["tweenEasing"].GetUint();
								if (Animation2D_bone_translateFrame[z].HasMember("x"))
									Animation2D_bone_translateFrame_current.x = Animation2D_bone_translateFrame[z]["x"].GetFloat();
								if (Animation2D_bone_translateFrame[z].HasMember("y"))
									Animation2D_bone_translateFrame_current.y = Animation2D_bone_translateFrame[z]["y"].GetFloat();

								Animation2D_bone_translateFrame_current.startTime = translateFrameStartTime;
								translateFrameStartTime += Animation2D_bone_translateFrame_current.duration;
								Animation2D_bone_current.translateFrame.push_back(Animation2D_bone_translateFrame_current);
							}
						}
						if (Animation2D_bone[l].HasMember("rotateFrame"))
						{
							const rapidjson::Value& Animation2D_bone_rotateFrame = Animation2D_bone[l]["rotateFrame"];
							float rotateFrameStartTime = 0.f;
							for (unsigned z = 0u; z < Animation2D_bone_rotateFrame.Size(); ++z)
							{
								Skeleton::Armature::Anim::Bone::RotateFrame Animation2D_bone_rotateFrame_current = {};

								if (Animation2D_bone_rotateFrame[z].HasMember("duration")) Animation2D_bone_rotateFrame_current.duration = Animation2D_bone_rotateFrame[z]["duration"].GetFloat();
								if (Animation2D_bone_rotateFrame[z].HasMember("tweenEasing")) Animation2D_bone_rotateFrame_current.tweenEasing = Animation2D_bone_rotateFrame[z]["tweenEasing"].GetUint();
								if (Animation2D_bone_rotateFrame[z].HasMember("rotate")) Animation2D_bone_rotateFrame_current.rotate = Animation2D_bone_rotateFrame[z]["rotate"].GetFloat();

								Animation2D_bone_rotateFrame_current.startTime = rotateFrameStartTime;
								rotateFrameStartTime += Animation2D_bone_rotateFrame_current.duration;
								Animation2D_bone_current.rotateFrame.push_back(Animation2D_bone_rotateFrame_current);
							}
						}

						Animation2D_current.bone.insert({ Animation2D_bone_current.name, Animation2D_bone_current });
					}
				} // Bone
				// Default Animation2D will be the first loaded
				if (i == 0u && j == 0u) m_CurrentAnimation = Animation2D_current.name;

				armature_current.animation.insert({ Animation2D_current.name, Animation2D_current });
			}
		}// Animation2D

		// Default Actions
		if (armature[i].HasMember("defaultActions"))
		{
			const rapidjson::Value& defaultaction = armature[i]["defaultActions"];
			for (unsigned j = 0u; j < defaultaction.Size(); ++j)
			{
				Skeleton::Armature::DefaultActions defaultActions_current = {};

				if (defaultaction[j].HasMember("gotoAndPlay"))
					defaultActions_current.gotoAndPlay = defaultaction[j]["gotoAndPlay"].GetString();

				armature_current.defaultActions.push_back(defaultActions_current);
			}
		}

		// Push the armature
		m_Skeleton.armature.push_back(armature_current);
	}

	// Once all the data is loaded, determine whether this is a rigged Animation2D or not
	m_IsRigged = !(/*m_Skeleton.armature.size() == 1 &&*/ m_Skeleton.armature.back().type == "Sheet");
}

const gef::Matrix33 AsdfAnim::Animation2D::BuildRigWorldTransform(const std::string& startBoneName)
{
	gef::Matrix33 worldTransform;
	worldTransform.SetIdentity();
	Skeleton::Armature::Bone* bone = &m_Skeleton.armature[m_CurrentArmature].bone.at(startBoneName);

	while (true)
	{
		const Skeleton::Armature::Anim& currentAnim = m_Skeleton.armature[m_CurrentArmature].animation[m_CurrentAnimation];

		// If there is Animation2D data for this bone
		gef::Vector2 currentAnimation2DTranslation(0.f, 0.f);
		float currentAnimaitonRotation = 0.f;
		if (currentAnim.bone.find(bone->name) != currentAnim.bone.end())
		{
			// Get Animation2D translation
			const size_t translateArraySize = currentAnim.bone.at(bone->name).translateFrame.size();
			for (size_t i = 0; i < translateArraySize; ++i)
			{
				const size_t next = i == translateArraySize - 1 ? 0u : i + 1;
				const auto& currentItem = currentAnim.bone.at(bone->name).translateFrame[i];
				const auto& nextItem = currentAnim.bone.at(bone->name).translateFrame[next];
				const float& currentStartTime = currentItem.startTime * 1.f / m_Skeleton.armature[m_CurrentArmature].frameRate;	// TODO: Load as floats
				const float& nextStartTime = nextItem.startTime * 1.f / m_Skeleton.armature[m_CurrentArmature].frameRate;
				if (m_Clock > currentStartTime && m_Clock < nextStartTime)
				{
					const float time = (m_Clock - currentStartTime) / (nextStartTime - currentStartTime);
					currentAnimation2DTranslation.x = gef::Lerp(currentItem.x, nextItem.x, time);
					currentAnimation2DTranslation.y = gef::Lerp(currentItem.y, nextItem.y, time);
					break;
				}
			}
			// Get Animation2D rotation
			const size_t rotateArraySize = currentAnim.bone.at(bone->name).rotateFrame.size();
			for (size_t i = 0; i < rotateArraySize; ++i)
			{
				const size_t next = i == rotateArraySize - 1 ? 0u : i + 1;
				const auto& currentItem = currentAnim.bone.at(bone->name).rotateFrame[i];
				const auto& nextItem = currentAnim.bone.at(bone->name).rotateFrame[next];
				const float& currentStartTime = currentItem.startTime * 1.f / m_Skeleton.armature[m_CurrentArmature].frameRate;	// TODO: Load as floats
				const float& nextStartTime = nextItem.startTime * 1.f / m_Skeleton.armature[m_CurrentArmature].frameRate;
				if (m_Clock > currentStartTime && m_Clock < nextStartTime)
				{
					const float time = (m_Clock - currentStartTime) / (nextStartTime - currentStartTime);
					currentAnimaitonRotation = gef::LerpRot(currentItem.rotate, nextItem.rotate, time);
					break;
				}
			}
		}

		// Build the local world matrix
		gef::Matrix33 localWorld = gef::Matrix33::kIdentity;
		localWorld.Rotate(DEG_TO_RAD(bone->transform.skX + currentAnimaitonRotation));
		//float temp = bone->transform.x + currentAnimation2DTranslation.x;
		//temp = bone->transform.y + currentAnimation2DTranslation.y;
		localWorld.SetTranslation(gef::Vector2(bone->transform.x + currentAnimation2DTranslation.x, bone->transform.y + currentAnimation2DTranslation.y));
		worldTransform = worldTransform * localWorld;

		if (!bone->parent.empty()) bone = &m_Skeleton.armature[m_CurrentArmature].bone.at(bone->parent);
		else break;
	}
	return std::move(worldTransform);
}