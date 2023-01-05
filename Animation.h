#pragma once
namespace gef
{
	class Animation;
}

namespace AsdfAnim
{
	enum class AnimationType {
		Animation_Type_Undefined = 0,
		Animation_Type_2D,
		Animation_Type_2D_Rigged,
		Animation_Type_3D
	};

	enum class ClipType {
		Clip_Type_Undefined = 0,
		Clip_Type_Idle,
		Clip_Type_Walk,
		Clip_Type_Run,
		Clip_Type_Jump,
		Clip_Type_Fall
	};

	enum class TransitionType {
		Transition_Type_Undefined = 0,
		Transition_Type_Frozen,
		Transition_Type_Smooth
	};

	struct Clip {
		gef::Animation* clip;
		ClipType type;
		std::string name;
		uint32_t id;
	};

	class Animation
	{
	public:
		Animation() : m_Active(false), m_Type(AnimationType::Animation_Type_Undefined) {}
		virtual void Update(float frameTime) = 0;

		void SetActive(bool active) { m_Active = active; }
		bool IsActive() const { return m_Active; }

		void SetType(AnimationType type) { m_Type = type; }
		bool IsType(AnimationType type) const { return m_Type == type; }
		AnimationType GetType() const { return m_Type; }

	private:
		bool m_Active;
		AnimationType m_Type;
	};
}