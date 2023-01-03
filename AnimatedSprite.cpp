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
}