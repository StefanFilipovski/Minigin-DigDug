#include "CircleMovementComponent.h"
#include "TransformComponent.h"
#include "GameObject.h"
#include <glm/gtc/constants.hpp>
#include <cmath>

namespace dae
{
	void CircleMovementComponent::Update(float deltaTime)
	{
		// Advance angle
		m_Angle += m_Speed * deltaTime;
		if (m_Angle > glm::two_pi<float>())
			m_Angle -= glm::two_pi<float>();

		// Compute world-space target position
		float worldX = m_Centre.x + std::cos(m_Angle) * m_Radius;
		float worldY = m_Centre.y + std::sin(m_Angle) * m_Radius;

		// If this object has a parent, convert world position → local position
		// so that TransformComponent::GetWorldPosition() still produces the
		// correct result. For a root object (no parent) local == world.
		glm::vec3 parentWorld{ 0.f, 0.f, 0.f };
		GameObject* parent = GetOwner()->GetParent();
		if (parent)
		{
			auto* parentTransform = parent->GetComponent<TransformComponent>();
			if (parentTransform)
				parentWorld = parentTransform->GetWorldPosition();
		}

		auto* transform = GetOwner()->GetComponent<TransformComponent>();
		if (transform)
			transform->SetLocalPosition(worldX - parentWorld.x,
				worldY - parentWorld.y,
				m_Centre.z);
	}
}