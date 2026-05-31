#pragma once
#include "Command.h"
#include "EnemyComponent.h"
#include "GameObject.h"

namespace dae
{
	// Versus: triggers the player-controlled Fygar's ghost form so it can
	// phase through dirt (subject to a cooldown).
	class GhostCommand final : public Command
	{
	public:
		explicit GhostCommand(GameObject* pFygar)
			: m_pFygar(pFygar)
		{
		}

		void Execute() override
		{
			if (auto* enemy = m_pFygar->GetComponent<EnemyComponent>())
				enemy->StartGhost();
		}

	private:
		GameObject* m_pFygar;
	};
}
