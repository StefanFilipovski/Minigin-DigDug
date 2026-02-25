#pragma once
#include "Component.h"
#include <glm/glm.hpp>

namespace dae
{
	// Moves the owning GameObject in a circle around a fixed world-space centre.
	// Completely independent of the parent/child hierarchy — it simply sets
	// the object's LOCAL position each frame so that its WORLD position traces
	// a circle around 'centre'.
	//
	// Usage:
	//   charA->AddComponent<dae::CircleMovementComponent>(
	//       glm::vec3{512.f, 288.f, 0.f},   // centre of the circle
	//       150.f,                           // radius in pixels
	//       1.0f                             // angular speed in radians/second
	//   );
	class CircleMovementComponent final : public Component
	{
	public:
		explicit CircleMovementComponent(GameObject* pOwner,
			const glm::vec3& centre = { 0.f, 0.f, 0.f },
			float radius = 100.f,
			float speed = 1.f)
			: Component(pOwner)
			, m_Centre(centre)
			, m_Radius(radius)
			, m_Speed(speed)
		{
		}

		void Update(float deltaTime) override;

		void SetCentre(const glm::vec3& centre) { m_Centre = centre; }
		void SetRadius(float radius) { m_Radius = radius; }
		void SetSpeed(float speed) { m_Speed = speed; }

		float GetAngle() const { return m_Angle; }

	private:
		glm::vec3 m_Centre;
		float     m_Radius;
		float     m_Speed;
		float     m_Angle{ 0.f };
	};
}