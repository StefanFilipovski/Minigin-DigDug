#pragma once
#include "Command.h"
#include "EnemyComponent.h"
#include "GameObject.h"

namespace dae
{
	class FireBreathCommand final : public Command
	{
	public:
		explicit FireBreathCommand(GameObject* pFygar)
			: m_pFygar(pFygar)
		{
		}

		void Execute() override
		{
			if (auto* enemy = m_pFygar->GetComponent<EnemyComponent>())
				enemy->StartFireBreath();
		}

	private:
		GameObject* m_pFygar;
	};
}
