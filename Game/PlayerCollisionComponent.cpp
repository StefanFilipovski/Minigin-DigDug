#include "PlayerCollisionComponent.h"
#include "GridMovementComponent.h"
#include "PumpComponent.h"
#include "ServiceLocator.h"
#include <algorithm>
#include "GridComponent.h"
#include "EnemyComponent.h"
#include "RenderComponent.h"
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
			m_cached = true;
		}

		if (!m_pMovement) return;

		// Prune enemies destroyed last frame (prevents dangling pointers)
		m_enemies.erase(
			std::remove_if(m_enemies.begin(), m_enemies.end(),
				[](const GameObject* go) { return !go || go->IsMarkedForDestroy(); }),
			m_enemies.end());

		// If dead, count down respawn timer
		if (m_dead)
		{
			m_respawnTimer -= deltaTime;
			if (m_respawnTimer <= 0.f)
			{
				Respawn(m_spawnPos.x, m_spawnPos.y);
			}
			return;
		}

		// Check collision with each enemy
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
		m_respawnTimer = m_respawnDelay;

		ServiceLocator::GetSoundService().PlaySound("Data/death.wav");

		auto* pump = GetOwner()->GetComponent<PumpComponent>();
		if (pump) pump->ForceReset();

		auto* render = GetOwner()->GetComponent<RenderComponent>();
		if (render) render->SetSourceRect(0, 0, 0, 0);

		NotifyObservers(EVENT_PLAYER_DIED, GetOwner());
	}

	void PlayerCollisionComponent::Respawn(int gridX, int gridY)
	{
		m_dead = false;

		if (m_pMovement)
		{
			m_pMovement->SetGridPosition(gridX, gridY);
		}

		// Show the player again
		// The SpriteAnimatorComponent will set the correct source rect on next frame
		auto* render = GetOwner()->GetComponent<RenderComponent>();
		if (render) render->ClearSourceRect();
	}
		
}