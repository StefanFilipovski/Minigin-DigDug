#include "HealthComponent.h"
#include "GameObject.h"
#include "GameEventIds.h"

namespace dae
{
	HealthComponent::HealthComponent(GameObject* pOwner, int lives)
		: Component(pOwner)
		, m_Lives(lives)
	{
	}

	void HealthComponent::Die()
	{
		if (m_Lives <= 0) return;
		--m_Lives;
		NotifyObservers(EVENT_PLAYER_DIED, GetOwner());
	}
}