#include "AnimatedSprite.h"
#include "gef_texture_loader.h"
#include "system/platform.h"

namespace gef
{
	AnimatedSprite::AnimatedSprite(gef::Platform& platform, const char* texturePath) :
		m_BodyPosition(gef::Vector2(platform.width() * 0.5f, platform.height() * 0.5f)),
		m_BodyRotation(0.f),
		m_BodyScale(1.f, 1.f)
	{
		// Init the texture
		this->set_texture(CreateTextureFromPNG(texturePath, platform));
	}

	AnimatedSprite::~AnimatedSprite()
	{
	}

	void AnimatedSprite::SetBodyPosition(const gef::Vector2& pos)
	{
		m_BodyPosition = pos;
	}

	const gef::Vector2& AnimatedSprite::GetBodyPosition() const
	{
		return m_BodyPosition;
	}
	void AnimatedSprite::SetBodyRotation(float rotation)
	{
		m_BodyRotation = rotation;
	}
	const float& AnimatedSprite::GetBodyRotation() const
	{
		return m_BodyRotation;
	}
	void AnimatedSprite::SetBodyScale(const gef::Vector2& scale)
	{
		m_BodyScale = scale;
	}
	const gef::Vector2& AnimatedSprite::GetBodyScale() const
	{
		return m_BodyScale;
	}
}