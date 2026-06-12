#include "FloatingScoreComponent.h"
#include "TransformComponent.h"
#include "GameObject.h"

namespace dae
{
	FloatingScoreComponent::FloatingScoreComponent(GameObject* pOwner,
		float lifetime, float riseSpeed)
		: Component(pOwner)
		, m_lifetime(lifetime)
		, m_riseSpeed(riseSpeed)
	{
	}

	void FloatingScoreComponent::Activate()
	{
		m_timer = 0.f;
		m_active = true;
	}

	void FloatingScoreComponent::Update(float deltaTime)
	{
		if (!m_active) return;

		if (!m_cached)
		{
			m_pTransform = GetOwner()->GetComponent<TransformComponent>();
			m_cached = true;
		}

		m_timer += deltaTime;

		if (m_pTransform)
		{
			auto pos = m_pTransform->GetLocalPosition();
			pos.y -= m_riseSpeed * deltaTime;
			m_pTransform->SetLocalPosition(pos.x, pos.y);
		}

		// Lifetime over — park offscreen and go back to the pool
		if (m_timer >= m_lifetime)
		{
			m_active = false;
			if (m_pTransform)
				m_pTransform->SetLocalPosition(-1000.f, -1000.f);
		}
	}
}
