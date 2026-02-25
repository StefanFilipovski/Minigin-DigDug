#include "RotatorComponent.h"
#include "TransformComponent.h"
#include "GameObject.h"
#include <glm/gtc/constants.hpp>
#include <cmath>

namespace dae
{
	void RotatorComponent::Update(float deltaTime)
	{
		m_Angle += m_Speed * deltaTime;
		if (m_Angle > glm::two_pi<float>())
			m_Angle -= glm::two_pi<float>();

		// Compute position in local space (i.e. relative to parent)
		float x = std::cos(m_Angle) * m_Radius;
		float y = std::sin(m_Angle) * m_Radius;

		auto* transform = GetOwner()->GetComponent<TransformComponent>();
		if (transform)
			transform->SetLocalPosition(x, y, 0.f);
	}
}