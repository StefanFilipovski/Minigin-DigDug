#include "ScoreComponent.h"
#include "GameObject.h"
#include "GameEventIds.h"

namespace dae
{
	ScoreComponent::ScoreComponent(GameObject* pOwner)
		: Component(pOwner)
	{
	}

	void ScoreComponent::AddPoints(int points)
	{
		m_Score += points;
		NotifyObservers(EVENT_POINTS_GAINED, GetOwner());
	}
}