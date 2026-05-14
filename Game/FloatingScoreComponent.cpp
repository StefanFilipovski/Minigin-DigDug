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

	void FloatingScoreComponent::Update(float deltaTime)
	{
		if (!m_cached)
		{
			m_pTransform = GetOwner()->GetComponent<TransformComponent>();
			m_cached = true;
		}

		m_timer += deltaTime;

		// Float upward
		if (m_pTransform)
		{
			auto pos = m_pTransform->GetLocalPosition();
			pos.y -= m_riseSpeed * deltaTime;
			m_pTransform->SetLocalPosition(pos.x, pos.y);
		}

		// Self-destruct after lifetime
		if (m_timer >= m_lifetime)
		{
			GetOwner()->MarkForDestroy();
		}
	}
}
