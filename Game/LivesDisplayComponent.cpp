#include "LivesDisplayComponent.h"
#include "TextComponent.h"
#include "PlayerCollisionComponent.h"
#include "GameObject.h"
#include "GameEventIds.h"
#include <string>

namespace dae
{
	LivesDisplayComponent::LivesDisplayComponent(GameObject* pOwner, const std::string& prefix)
		: Component(pOwner)
		, m_prefix(prefix)
	{
	}

	void LivesDisplayComponent::Notify(EventId event, GameObject* pGameObject)
	{
		if (event != EVENT_PLAYER_DIED) return;
		if (!pGameObject) return;

		auto* pCollision = pGameObject->GetComponent<PlayerCollisionComponent>();
		if (!pCollision) return;

		auto* pText = GetOwner()->GetComponent<TextComponent>();
		if (!pText) return;

		pText->SetText(m_prefix + std::to_string(pCollision->GetLives()));
	}
}