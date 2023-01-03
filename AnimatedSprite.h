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

		void SetBodyPosition(const gef::Vector2& pos) { m_BodyPosition = pos; }
		const gef::Vector2& GetBodyPosition() const { return m_BodyPosition; }

		void SetBodyRotation(float rotation) { m_BodyRotation = rotation; }
		const float& GetBodyRotation() const { return m_BodyRotation; }

		void SetBodyScale(const gef::Vector2& scale) { m_BodyScale = scale; }
		const gef::Vector2& GetBodyScale() const { return m_BodyScale; }

	private:
		// gef::Sprite already has a position component, but that will be used to position different parts of the sprite
		// The m_BodyPosition defines where in the world the sprite should be (i.e. the overall position)
		gef::Vector2 m_BodyPosition;
		float m_BodyRotation;
		gef::Vector2 m_BodyScale;
	};
}

#endif