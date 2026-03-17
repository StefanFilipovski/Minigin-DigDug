#include "PointsDisplayComponent.h"
#include "TextComponent.h"
#include "ScoreComponent.h"
#include "GameObject.h"
#include "EventIds.h"
#include <string>

namespace dae
{
	PointsDisplayComponent::PointsDisplayComponent(GameObject* pOwner)
		: Component(pOwner)
	{
	}

	void PointsDisplayComponent::Notify(EventId event, GameObject* pGameObject)
	{
		if (event != EVENT_POINTS_GAINED) return;
		if (!pGameObject) return;

		auto* pScore = pGameObject->GetComponent<ScoreComponent>();
		if (!pScore) return;

		auto* pText = GetOwner()->GetComponent<TextComponent>();
		if (!pText) return;

		pText->SetText("Score: " + std::to_string(pScore->GetScore()));
	}
}