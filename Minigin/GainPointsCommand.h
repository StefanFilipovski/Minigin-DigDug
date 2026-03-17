#pragma once
#include "Command.h"
#include "ScoreComponent.h"
#include "GameObject.h"

namespace dae
{
	class GainPointsCommand final : public GameActorCommand
	{
	public:
		GainPointsCommand(GameObject* pGameObject, int points)
			: GameActorCommand(pGameObject)
			, m_Points(points) {
		}

		void Execute() override
		{
			auto* pScore = GetGameActor()->GetComponent<ScoreComponent>();
			if (pScore) pScore->AddPoints(m_Points);
		}

	private:
		int m_Points;
	};
}