#include "PumpComponent.h"
#include "PumpHoseComponent.h"
#include "GridComponent.h"
#include "GridMovementComponent.h"
#include "SpriteAnimatorComponent.h"
#include "EnemyComponent.h"
#include "PlayerCollisionComponent.h"
#include "RenderComponent.h"
#include "TransformComponent.h"
#include "Renderer.h"
#include "ResourceManager.h"
#include "Texture2D.h"
#include "GameObject.h"
#include "ServiceLocator.h"
#include "GameSounds.h"
#include <algorithm>

namespace dae
{
	PumpComponent::PumpComponent(GameObject* pOwner, GridComponent* pGrid)
		: Component(pOwner)
		, m_pGrid(pGrid)
	{
	}

	void PumpComponent::CacheComponents()
	{
		if (m_cached) return;
		m_pMovement = GetOwner()->GetComponent<GridMovementComponent>();
		m_pAnimator = GetOwner()->GetComponent<SpriteAnimatorComponent>();
		m_pCollision = GetOwner()->GetComponent<PlayerCollisionComponent>();
		m_cached = true;
	}

	void PumpComponent::AddEnemy(GameObject* pEnemy)
	{
		m_enemies.push_back(pEnemy);
	}

	void PumpComponent::ClearEnemies()
	{
		m_enemies.clear();
		m_pLatchedEnemyGO = nullptr;
	}

	void PumpComponent::Fire()
	{
		CacheComponents();
		if (!m_pMovement) return;

		// Don't allow pumping while dead
		if (m_pCollision && m_pCollision->IsDead()) return;

		// Mark button as held this frame 
		m_fireButtonHeld = true;

		// While latched onto an enemy: tap = immediate pump, hold = handled in Update
		if (m_state == PumpState::Latched)
		{
			// Fresh press
			if (!m_fireButtonHeldPrev)
			{
				if (m_pLatchedEnemyGO && !m_pLatchedEnemyGO->IsMarkedForDestroy())
				{
					auto* enemy = m_pLatchedEnemyGO->GetComponent<EnemyComponent>();
					if (enemy) enemy->PumpOnce();
				}
				m_holdPumpTimer = 0.f; // reset hold timer on fresh press
			}
			return;
		}

		if (m_pMovement->IsMoving()) return;

		if (m_state == PumpState::Idle)
		{
			m_fireDirection = m_pMovement->GetCurrentDirection();
			if (m_fireDirection == glm::ivec2{ 0, 0 })
				m_fireDirection = { 1, 0 };

			m_hoseLength = 0.f;
			m_state = PumpState::Extending;

			ServiceLocator::GetSoundService().PlaySound(Sounds::PumpShot);

			CalculateRange();

			if (m_pAnimator)
			{
				if (m_fireDirection.x > 0) m_pAnimator->Play("pump_right");
				else if (m_fireDirection.x < 0) m_pAnimator->Play("pump_left");
				else if (m_fireDirection.y < 0) m_pAnimator->Play("pump_up");
				else if (m_fireDirection.y > 0) m_pAnimator->Play("pump_down");
			}
		}
	}

	void PumpComponent::Update(float deltaTime)
	{
		CacheComponents();

		if (m_pCollision && m_pCollision->IsDead())
		{
			// Still clear held state so we don't get a stale "held" on respawn
			m_fireButtonHeld = false;
			m_fireButtonHeldPrev = false;
			return;
		}

		
		// Safe here because Scene::Update() calls all object Updates before freeing marked objects.
		m_enemies.erase(
			std::remove_if(m_enemies.begin(), m_enemies.end(),
				[](const GameObject* go) { return !go || go->IsMarkedForDestroy(); }),
			m_enemies.end());

		if (m_pLatchedEnemyGO && m_pLatchedEnemyGO->IsMarkedForDestroy())
			m_pLatchedEnemyGO = nullptr;

		switch (m_state)
		{
		case PumpState::Idle:
			break;

		case PumpState::Extending:
		{
			m_hoseLength += m_extendSpeed * deltaTime;

			// Check every cell the hose tip has reached for an enemy
			if (m_pMovement)
			{
				const auto& playerPos = m_pMovement->GetGridPosition();
				int tipCell = static_cast<int>(std::ceil(m_hoseLength));
				tipCell = std::min(tipCell, m_currentRange);

				for (int i = 1; i <= tipCell && m_state == PumpState::Extending; ++i)
				{
					glm::ivec2 checkPos = playerPos + m_fireDirection * i;

					for (auto* pEnemyGO : m_enemies)
					{
						auto* enemy = pEnemyGO->GetComponent<EnemyComponent>();
						if (!enemy || !enemy->IsAlive() || enemy->IsInflating()
							|| enemy->IsGhost()) continue;

						if (enemy->GetGridPosition() == checkPos)
						{
							m_pLatchedEnemyGO = pEnemyGO;
							m_hoseLength = static_cast<float>(i);
							m_state = PumpState::Latched;
							enemy->StartInflating(m_fireDirection);
							break;
						}
					}
				}
			}

			if (m_state == PumpState::Extending &&
				m_hoseLength >= static_cast<float>(m_currentRange))
			{
				m_hoseLength = static_cast<float>(m_currentRange);
				m_state = PumpState::Retracting;
				m_retractTimer = RetractDelay;
			}
			break;
		}

		case PumpState::Latched:
		{
			// Release if the enemy died, was destroyed, or deflated back to normal
			bool shouldRelease = !m_pLatchedEnemyGO;
			if (!shouldRelease)
			{
				auto* enemy = m_pLatchedEnemyGO->GetComponent<EnemyComponent>();
				shouldRelease = !enemy || !enemy->IsAlive() || !enemy->IsInflating();
			}

			if (shouldRelease)
			{
				m_pLatchedEnemyGO = nullptr;
				m_state = PumpState::Retracting;
				m_retractTimer = 0.f;
			}
			else if (m_fireButtonHeld)
			{
				// Hold-to-pump: auto-inflate at a slower rate than tapping
				m_holdPumpTimer += deltaTime;
				if (m_holdPumpTimer >= HoldPumpInterval)
				{
					m_holdPumpTimer -= HoldPumpInterval;
					if (m_pLatchedEnemyGO && !m_pLatchedEnemyGO->IsMarkedForDestroy())
					{
						auto* enemy = m_pLatchedEnemyGO->GetComponent<EnemyComponent>();
						if (enemy) enemy->PumpOnce();
					}
				}
			}
			break;
		}

		case PumpState::Retracting:
		{
			m_retractTimer -= deltaTime;
			if (m_retractTimer > 0.f)
				break;

			m_hoseLength -= m_retractSpeed * deltaTime;

			if (m_hoseLength <= 0.f)
			{
				m_hoseLength = 0.f;
				m_state = PumpState::Idle;

				if (m_pAnimator)
				{
					if (m_fireDirection.x > 0) m_pAnimator->Play("walk_right");
					else if (m_fireDirection.x < 0) m_pAnimator->Play("walk_left");
					else if (m_fireDirection.y < 0) m_pAnimator->Play("walk_up");
					else if (m_fireDirection.y > 0) m_pAnimator->Play("walk_down");
					m_pAnimator->Pause();
				}
			}
			break;
		}
		}

		// Track button state for tap vs hold detection
		m_fireButtonHeldPrev = m_fireButtonHeld;
		m_fireButtonHeld = false;

		SyncHose();
	}

	// Push the current pump state to the hose visual (a child GameObject of
	// the player — its position follows the player via the scene graph)
	void PumpComponent::SyncHose()
	{
		if (!m_pHose) return;

		if (m_state == PumpState::Idle || m_hoseLength <= 0.f)
			m_pHose->Hide();
		else
			m_pHose->Show(m_fireDirection, m_hoseLength, m_currentRange);
	}

	void PumpComponent::ForceReset()
	{
		m_state = PumpState::Idle;
		m_hoseLength = 0.f;
		m_pLatchedEnemyGO = nullptr;
		m_fireButtonHeld = false;
		m_fireButtonHeldPrev = false;
		m_holdPumpTimer = 0.f;

		if (m_pHose) m_pHose->Hide();
	}

	void PumpComponent::CalculateRange()
	{
		if (!m_pMovement || !m_pGrid) return;

		const auto& gridPos = m_pMovement->GetGridPosition();
		m_currentRange = 0;

		for (int i = 1; i <= m_maxRange; ++i)
		{
			glm::ivec2 checkPos = gridPos + m_fireDirection * i;

			if (!m_pGrid->IsInBounds(checkPos.x, checkPos.y))
				break;

			CellType cellType = m_pGrid->GetCellType(checkPos.x, checkPos.y);

			if (cellType == CellType::Dirt)
				break;

			m_currentRange = i;
		}

		if (m_currentRange == 0)
			m_currentRange = 1;
	}
}