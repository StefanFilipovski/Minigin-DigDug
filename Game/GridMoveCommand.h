#pragma once
#include "Command.h"
#include "GridMovementComponent.h"
#include "GameObject.h"
#include <glm/glm.hpp>

namespace dae
{
	class GridMoveCommand final : public Command
	{
	public:
		GridMoveCommand(GameObject* pActor, const glm::ivec2& direction)
			: m_pActor(pActor)
			, m_direction(direction)
		{
		}

		void Execute() override
		{
			if (auto* movement = m_pActor->GetComponent<GridMovementComponent>())
				movement->SetDesiredDirection(m_direction);
		}

	private:
		GameObject* m_pActor;
		glm::ivec2 m_direction;
	};

	// Stops movement when a direction key is released
	class GridStopCommand final : public Command
	{
	public:
		GridStopCommand(GameObject* pActor, const glm::ivec2& direction)
			: m_pActor(pActor)
			, m_direction(direction)
		{
		}

		void Execute() override
		{
			if (auto* movement = m_pActor->GetComponent<GridMovementComponent>())
			{
				// Only stop if we're still moving in this direction
				if (movement->GetCurrentDirection() == m_direction ||
					!movement->IsMoving())
				{
					movement->SetDesiredDirection({ 0, 0 });
				}
			}
		}

	private:
		GameObject* m_pActor;
		glm::ivec2 m_direction;
	};
}