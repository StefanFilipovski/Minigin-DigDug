#include "LivesDisplayComponent.h"
#include "TextComponent.h"
#include "HealthComponent.h"
#include "GameObject.h"
#include "EventIds.h"
#include <string>

namespace dae
{
	LivesDisplayComponent::LivesDisplayComponent(GameObject* pOwner)
		: Component(pOwner)
	{
	}

	void LivesDisplayComponent::Notify(EventId event, GameObject* pGameObject)
	{
		if (event != EVENT_PLAYER_DIED) return;
		if (!pGameObject) return;

		auto* pHealth = pGameObject->GetComponent<HealthComponent>();
		if (!pHealth) return;

		auto* pText = GetOwner()->GetComponent<TextComponent>();
		if (!pText) return;

		pText->SetText("Lives: " + std::to_string(pHealth->GetLives()));
	}
}