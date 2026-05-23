#include "PointsDisplayComponent.h"
#include "TextComponent.h"
#include "ScoreComponent.h"
#include "GameObject.h"
#include "GameEventIds.h"
#include <string>

namespace dae
{
	PointsDisplayComponent::PointsDisplayComponent(GameObject* pOwner, const std::string& prefix)
		: Component(pOwner)
		, m_prefix(prefix)
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

		pText->SetText(m_prefix + std::to_string(pScore->GetScore()));
	}
}