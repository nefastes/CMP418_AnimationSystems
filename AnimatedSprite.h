#ifndef ANIMATED_SPRITE
#define ANIMATED_SPRITE

#include "graphics/sprite_renderer.h"
#include "graphics/sprite.h"
#include "maths/vector2.h"
#include "Animation2D.h"

namespace gef
{
	class AnimatedSprite : public gef::Sprite
	{
	public:
		AnimatedSprite(gef::Platform& platform, const char* texurePath);
		~AnimatedSprite();

		void SetBodyPosition(const gef::Vector2& pos);
		const gef::Vector2& GetBodyPosition() const;

		void SetBodyRotation(float rotation);
		const float& GetBodyRotation() const;

		void SetBodyScale(const gef::Vector2& scale);
		const gef::Vector2& GetBodyScale() const;

	private:
		// gef::Sprite already has a position component, but that will be used to position different parts of the sprite
		// The m_BodyPosition defines where in the world the sprite should be (i.e. the overall position)
		gef::Vector2 m_BodyPosition;
		float m_BodyRotation;
		gef::Vector2 m_BodyScale;
	};
}

#endif