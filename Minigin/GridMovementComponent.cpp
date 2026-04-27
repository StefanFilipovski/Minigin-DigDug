#include "GridMovementComponent.h"
#include "GridComponent.h"
#include "TransformComponent.h"
#include "PumpComponent.h"
#include "GameObject.h"

namespace dae
{
	GridMovementComponent::GridMovementComponent(GameObject* pOwner,
		GridComponent* pGrid, float speed, bool canDig)
		: Component(pOwner)
		, m_pGrid(pGrid)
		, m_speed(speed)
		, m_canDig(canDig)
	{
	}

	void GridMovementComponent::Update(float /*deltaTime*/)
	{
		// Called once per frame, after input processing.
		// If no direction key was pressed this frame, clear the desired direction.
		if (!m_inputActive)
			m_desiredDirection = { 0, 0 };

		// Reset for next frame — if a key is held, the Pressed command
		// will set it again before the next Update
		m_inputActive = false;
	}

	void GridMovementComponent::FixedUpdate(float fixedTimeStep)
	{
		if (!m_pTransform)
		{
			m_pTransform = GetOwner()->GetComponent<TransformComponent>();
			if (!m_pTransform) return;
		}

		if (m_isMoving)
		{
			m_moveProgress += m_speed * fixedTimeStep;

			if (m_moveProgress >= 1.f)
			{
				m_moveProgress = 1.f;
				m_isMoving = false;
				m_gridPos = m_targetGridPos;
				m_pixelPos = m_targetPixel;

				ArriveAtCell();
			}
			else
			{
				m_pixelPos = m_startPixel + (m_targetPixel - m_startPixel) * m_moveProgress;
			}

			m_pTransform->SetLocalPosition(m_pixelPos.x, m_pixelPos.y);
		}

		// Only start a new move if a direction is actively being requested
		// and the pump is not active
		if (!m_isMoving && m_desiredDirection != glm::ivec2{ 0, 0 })
		{
			// Don't move while the pump is firing
			auto* pump = GetOwner()->GetComponent<PumpComponent>();
			if (pump && pump->IsFiring())
				return;
			glm::ivec2 target = m_gridPos + m_desiredDirection;
			if (CanMoveTo(target, m_desiredDirection))
			{
				m_currentDirection = m_desiredDirection;
				m_targetGridPos = target;
				m_isMoving = true;
				m_moveProgress = 0.f;
				m_startPixel = m_pGrid->GridToPixel(m_gridPos.x, m_gridPos.y);
				m_targetPixel = m_pGrid->GridToPixel(m_targetGridPos.x, m_targetGridPos.y);

				// Check if we're digging into dirt before we convert it
				m_isDigging = m_canDig &&
					m_pGrid->GetCellType(m_targetGridPos.x, m_targetGridPos.y) == CellType::Dirt;

				if (m_canDig)
					m_pGrid->DigCell(m_targetGridPos.x, m_targetGridPos.y, m_currentDirection);
			}
		}
	}

	void GridMovementComponent::SetDesiredDirection(const glm::ivec2& direction)
	{
		m_desiredDirection = direction;
		m_inputActive = true;
	}

	void GridMovementComponent::SetGridPosition(int gridX, int gridY)
	{
		m_gridPos = { gridX, gridY };
		m_targetGridPos = m_gridPos;
		m_isMoving = false;
		m_moveProgress = 0.f;

		if (m_pGrid)
		{
			m_pixelPos = m_pGrid->GridToPixel(gridX, gridY);
			m_startPixel = m_pixelPos;
			m_targetPixel = m_pixelPos;
		}

		if (m_pTransform)
			m_pTransform->SetLocalPosition(m_pixelPos.x, m_pixelPos.y);
	}

	bool GridMovementComponent::CanMoveTo(const glm::ivec2& targetCell,
		const glm::ivec2& direction) const
	{
		if (!m_pGrid->IsInBounds(targetCell.x, targetCell.y))
			return false;

		if (m_isGhost)
			return true;

		if (m_canDig)
			return true;

		const auto& cell = m_pGrid->GetCell(targetCell.x, targetCell.y);
		return cell.IsPassableFrom(direction);
	}

	void GridMovementComponent::ArriveAtCell()
	{
		if (m_canDig)
			m_pGrid->DigCell(m_gridPos.x, m_gridPos.y, m_currentDirection);
	}
}