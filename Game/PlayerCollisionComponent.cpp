#include "PlayerCollisionComponent.h"
#include "GridMovementComponent.h"
#include "SpriteAnimatorComponent.h"
#include "PumpComponent.h"
#include "ServiceLocator.h"
#include <algorithm>
#include "GridComponent.h"
#include "EnemyComponent.h"
#include "RenderComponent.h"
#include "TransformComponent.h"
#include "GameEventIds.h"
#include "GameObject.h"

namespace dae
{
	PlayerCollisionComponent::PlayerCollisionComponent(GameObject* pOwner, GridComponent* pGrid)
		: Component(pOwner)
		, m_pGrid(pGrid)
	{
	}

	void PlayerCollisionComponent::Update(float deltaTime)
	{
		if (!m_cached)
		{
			m_pMovement = GetOwner()->GetComponent<GridMovementComponent>();
			m_pAnimator = GetOwner()->GetComponent<SpriteAnimatorComponent>();
			m_cached = true;
		}

		if (!m_pMovement) return;

		// Prune enemies destroyed last frame (prevents dangling pointers)
		m_enemies.erase(
			std::remove_if(m_enemies.begin(), m_enemies.end(),
				[](const GameObject* go) { return !go || go->IsMarkedForDestroy(); }),
			m_enemies.end());

		// ---- Death state machine ----
		if (m_dead)
		{
			switch (m_deathPhase)
			{
			case DeathPhase::DyingAnimation:
			{
				// Wait for die animation to finish, then hold last frame briefly
				if (m_pAnimator && m_pAnimator->IsAnimationFinished())
				{
					m_phaseTimer += deltaTime;
					if (m_phaseTimer >= LastFrameHold)
					{
						// Hide the player sprite during black screen
						auto* render = GetOwner()->GetComponent<RenderComponent>();
						if (render) render->SetSourceRect(0, 0, 0, 0);

						m_deathPhase = DeathPhase::BlackScreen;
						m_phaseTimer = BlackScreenDuration;
					}
				}
				break;
			}
			case DeathPhase::BlackScreen:
			{
				m_phaseTimer -= deltaTime;
				if (m_phaseTimer <= 0.f)
				{
					m_deathPhase = DeathPhase::SoftReset;
				}
				break;
			}
			case DeathPhase::SoftReset:
			{
				SoftReset();
				break;
			}
			default:
				break;
			}
			return;
		}

		// ---- Normal: check collision with each enemy ----
		const auto& myPos = m_pMovement->GetGridPosition();

		for (auto* pEnemy : m_enemies)
		{
			if (!pEnemy || pEnemy->IsMarkedForDestroy()) continue;

			auto* enemy = pEnemy->GetComponent<EnemyComponent>();
			if (!enemy || !enemy->IsAlive()) continue;

			// Skip inflating enemies — they're safe to walk through
			if (enemy->IsInflating()) continue;

			// Grid collision — same cell
			bool hit = (enemy->GetGridPosition() == myPos);

			// Fire collision — check if player is in a Fygar's fire path
			if (!hit && enemy->IsPositionInFire(myPos))
				hit = true;

			if (hit)
			{
				TriggerDeath();
				return;
			}
		}
	}

	void PlayerCollisionComponent::AddEnemy(GameObject* pEnemy)
	{
		m_enemies.push_back(pEnemy);
	}

	void PlayerCollisionComponent::ClearEnemies()
	{
		m_enemies.clear();
	}

	void PlayerCollisionComponent::TriggerDeath()
	{
		if (m_dead) return;

		--m_lives;
		m_dead = true;

		ServiceLocator::GetSoundService().PlaySound("Data/death.wav");

		auto* pump = GetOwner()->GetComponent<PumpComponent>();
		if (pump) pump->ForceReset();

		// Stop and freeze player movement
		if (m_pMovement)
		{
			m_pMovement->SnapToCurrentCell();
			m_pMovement->SetFrozen(true);
		}

		// Play the death animation
		if (m_pAnimator)
		{
			m_pAnimator->Play("die");
			m_pAnimator->Resume(); // make sure it's not paused
		}

		m_deathPhase = DeathPhase::DyingAnimation;
		m_phaseTimer = 0.f;

		NotifyObservers(EVENT_PLAYER_DIED, GetOwner());
	}

	void PlayerCollisionComponent::SoftReset()
	{
		if (m_lives <= 0)
		{
			m_deathPhase = DeathPhase::None;
			if (m_gameOverCallback)
				m_gameOverCallback();
			return;
		}

		// Respawn player at spawn position
		Respawn(m_spawnPos.x, m_spawnPos.y);

		// Fire callback — PlayingState uses this to re-create enemies and rebind input
		if (m_softResetCallback)
			m_softResetCallback();
	}

	void PlayerCollisionComponent::Respawn(int gridX, int gridY)
	{
		m_dead = false;
		m_deathPhase = DeathPhase::None;
		m_phaseTimer = 0.f;

		if (m_pMovement)
		{
			m_pMovement->SetFrozen(false);
			m_pMovement->SetGridPosition(gridX, gridY);

			// Reposition transform to match
			glm::vec2 pos = m_pGrid->GridToPixel(gridX, gridY);
			auto* transform = GetOwner()->GetComponent<TransformComponent>();
			if (transform)
				transform->SetLocalPosition(pos.x, pos.y);
		}

		// Show the player again — animator will set the correct source rect
		auto* render = GetOwner()->GetComponent<RenderComponent>();
		if (render) render->ClearSourceRect();

		// Reset to idle animation
		if (m_pAnimator)
		{
			m_pAnimator->Play("walk_right");
			m_pAnimator->Pause();
		}
	}
}
