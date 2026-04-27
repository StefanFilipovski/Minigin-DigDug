#pragma once
#include "Command.h"
#include "PumpComponent.h"
#include "GameObject.h"

namespace dae
{
	class PumpCommand final : public Command
	{
	public:
		explicit PumpCommand(GameObject* pActor)
			: m_pActor(pActor)
		{
		}

		void Execute() override
		{
			if (auto* pump = m_pActor->GetComponent<PumpComponent>())
				pump->Fire();
		}

	private:
		GameObject* m_pActor;
	};
}