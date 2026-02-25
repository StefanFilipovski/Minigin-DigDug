#pragma once
#include "Component.h"
#include <glm/glm.hpp>

namespace dae
{
	// Rotates (orbits) its owning GameObject around the parent's world position.
	// The radius is the distance from the parent; the speed is in radians per second.
	class RotatorComponent final : public Component
	{
	public:
		explicit RotatorComponent(GameObject* pOwner, float radius = 100.f, float speed = 1.f)
			: Component(pOwner), m_Radius(radius), m_Speed(speed) {
		}

		void Update(float deltaTime) override;

	private:
		float m_Radius;
		float m_Speed;
		float m_Angle{ 0.f };
	};
}
