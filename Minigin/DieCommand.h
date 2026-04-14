#pragma once
#include "Command.h"
#include "HealthComponent.h"
#include "GameObject.h"

namespace dae
{
	class DieCommand final : public GameActorCommand
	{
	public:
		explicit DieCommand(GameObject* pGameObject)
			: GameActorCommand(pGameObject) {
		}

		void Execute() override
		{
			auto* pHealth = GetGameActor()->GetComponent<HealthComponent>();
			if (pHealth) pHealth->Die();
		}
	};
}