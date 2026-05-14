#include "RockComponent.h"
#include "GridComponent.h"
#include "GridMovementComponent.h"
#include "EnemyComponent.h"
#include "PlayerCollisionComponent.h"
#include "TransformComponent.h"
#include "ResourceManager.h"
#include "Renderer.h"
#include "Texture2D.h"
#include <SDL3/SDL_pixels.h>
#include "GameEventIds.h"
#include "GameObject.h"
// #include "ServiceLocator.h" // uncomment when rock_break.wav is added
#include <cmath>

namespace dae
{
	RockComponent::RockComponent(GameObject* pOwner, GridComponent* pGrid,
		int gridX, int gridY)
		: Component(pOwner)
		, m_pGrid(pGrid)
		, m_gridPos(gridX, gridY)
	{
		SDL_Color redKey{ 108, 7, 0, 255 };
		m_texture = ResourceManager::GetInstance().LoadTexture("rocks.png", redKey);
	}

	void RockComponent::CacheComponents()
	{
		if (m_cached) return;
		m_pTransform = GetOwner()->GetComponent<TransformComponent>();
		m_cached = true;
	}

	void RockComponent::Update(float deltaTime)
	{
		CacheComponents();
		if (!m_pTransform || !m_pGrid) return;

		switch (m_state)
		{
		case RockState::Idle:
			UpdateIdle();
			break;
		case RockState::Unstable:
			UpdateUnstable();
			break;
		case RockState::Jittering:
			UpdateJittering(deltaTime);
			break;
		case RockState::Falling:
			UpdateFalling(deltaTime);
			break;
		case RockState::Breaking:
			UpdateBreaking(deltaTime);
			break;
		case RockState::Destroyed:
			break;
		}
	}

	void RockComponent::Render() const
	{
		if (m_state == RockState::Destroyed || !m_pTransform || !m_texture) return;

		const auto& pos = m_pTransform->GetWorldPosition();
		float cellSize = static_cast<float>(m_pGrid->GetCellSize());

		// Pick the correct sprite frame
		int frame = 0;
		if (m_state == RockState::Breaking)
			frame = 1 + m_breakFrame; // frames 1, 2, 3

		SDL_FRect srcRect{
			static_cast<float>(frame * SpriteW), 0.f,
			static_cast<float>(SpriteW), static_cast<float>(SpriteH)
		};

		float drawX = pos.x + m_jitterOffset;
		float drawY = pos.y;

		Renderer::GetInstance().RenderTexture(*m_texture, drawX, drawY,
			srcRect, cellSize, cellSize);
	}

	void RockComponent::AddPlayer(GameObject* pPlayer)
	{
		if (pPlayer)
			m_players.push_back(pPlayer);
	}

	void RockComponent::AddEnemy(GameObject* pEnemy)
	{
		if (pEnemy)
			m_enemies.push_back(pEnemy);
	}

	void RockComponent::ClearEnemies()
	{
		m_enemies.clear();
	}

	// ---- State updates ----

	void RockComponent::UpdateIdle()
	{
		// Check if the cell directly below is dug out
		int belowY = m_gridPos.y + 1;
		if (!m_pGrid->IsInBounds(m_gridPos.x, belowY))
			return; // At bottom of grid, can't fall

		CellType cellBelow = m_pGrid->GetCellType(m_gridPos.x, belowY);
		if (cellBelow == CellType::Tunnel)
		{
			// Cell below is dug — check if player is underneath
			if (IsPlayerDirectlyBelow())
				m_state = RockState::Unstable;
			else
			{
				// No player underneath — start jittering immediately
				m_state = RockState::Jittering;
				m_jitterTimer = 0.f;
			}
		}
	}

	void RockComponent::UpdateUnstable()
	{
		// Stay unstable as long as the player is directly below
		// Once the player moves away, start jittering
		if (!IsPlayerDirectlyBelow())
		{
			m_state = RockState::Jittering;
			m_jitterTimer = 0.f;
		}
	}

	void RockComponent::UpdateJittering(float deltaTime)
	{
		m_jitterTimer += deltaTime;

		// Shake effect — oscillate left/right
		m_jitterOffset = std::sin(m_jitterTimer * m_jitterSpeed) * 2.f;

		if (m_jitterTimer >= m_jitterDuration)
		{
			// Start falling
			m_state = RockState::Falling;
			m_jitterOffset = 0.f;
			m_startFallRow = m_gridPos.y;
			m_playerGraceTimer = PlayerGraceDuration; // player immunity window

			// Store starting pixel Y for smooth falling
			glm::vec2 cellPixel = m_pGrid->GridToPixel(m_gridPos.x, m_gridPos.y);
			m_pixelY = cellPixel.y;
		}
	}

	void RockComponent::UpdateFalling(float deltaTime)
	{
		float cellSize = static_cast<float>(m_pGrid->GetCellSize());

		// Tick grace timer — player who triggered the fall gets time to escape
		if (m_playerGraceTimer > 0.f)
			m_playerGraceTimer -= deltaTime;

		// Move down smoothly
		m_pixelY += m_fallSpeed * deltaTime;
		m_pTransform->SetLocalPosition(
			m_pTransform->GetLocalPosition().x,
			m_pixelY
		);

		// Determine which grid row the rock's bottom edge is in
		glm::ivec2 currentGridPos = m_pGrid->PixelToGrid(
			m_pTransform->GetLocalPosition().x + cellSize * 0.5f,
			m_pixelY + cellSize * 0.5f
		);

		// Update grid position as rock falls
		m_gridPos.y = currentGridPos.y;

		// Check for enemy crushes at current position
		CheckEnemyCrush();

		// Only check player crush after grace period expires
		if (m_playerGraceTimer <= 0.f)
			CheckPlayerCrush();

		// Check if the cell below the current position is solid or out of bounds
		int nextRow = m_gridPos.y + 1;
		bool shouldStop = false;

		if (!m_pGrid->IsInBounds(m_gridPos.x, nextRow))
		{
			// Reached bottom of grid
			shouldStop = true;
		}
		else
		{
			CellType cellBelow = m_pGrid->GetCellType(m_gridPos.x, nextRow);
			if (cellBelow == CellType::Dirt)
			{
				// Hit solid ground — check if we've passed the cell center
				glm::vec2 cellCenter = m_pGrid->GridToPixelCenter(m_gridPos.x, m_gridPos.y);
				if (m_pixelY + cellSize * 0.5f >= cellCenter.y)
					shouldStop = true;
			}
		}

		if (shouldStop)
		{
			// Snap to the landing cell
			glm::vec2 landPixel = m_pGrid->GridToPixel(m_gridPos.x, m_gridPos.y);
			m_pTransform->SetLocalPosition(landPixel.x, landPixel.y);
			m_pixelY = landPixel.y;

			// Transition to breaking
			m_state = RockState::Breaking;
			m_breakTimer = 0.f;
			m_breakFrame = 0;

			// Notify observers that this rock finished crushing enemies
			// ScoreComponent reads GetCrushCount() to award the correct bonus
			if (m_crushCount > 0)
			{
				NotifyObservers(EVENT_ROCK_CRUSH_COMPLETE, GetOwner());
			}

			// TODO: add rock_break.wav sound file
			// ServiceLocator::GetSoundService().PlaySound("Data/rock_break.wav");
		}
	}

	void RockComponent::UpdateBreaking(float deltaTime)
	{
		m_breakTimer += deltaTime;

		// Cycle through break frames (1, 2, 3) over the break duration
		float frameTime = m_breakDuration / 3.f;
		m_breakFrame = static_cast<int>(m_breakTimer / frameTime);

		if (m_breakFrame > 2)
			m_breakFrame = 2;

		if (m_breakTimer >= m_breakDuration)
		{
			m_state = RockState::Destroyed;
			GetOwner()->MarkForDestroy();
		}
	}

	// ---- Helpers ----

	bool RockComponent::IsPlayerDirectlyBelow() const
	{
		for (auto* pPlayer : m_players)
		{
			if (!pPlayer || pPlayer->IsMarkedForDestroy()) continue;

			auto* movement = pPlayer->GetComponent<GridMovementComponent>();
			if (!movement) continue;

			const auto& playerPos = movement->GetGridPosition();
			const auto& targetPos = movement->GetTargetGridPosition();

			// Player is "under" the rock if they're in the same column
			// and anywhere below it — check both current AND target position,
			// because DigCell converts the cell to Tunnel before the player
			// actually arrives (grid pos hasn't updated yet).
			if (playerPos.x == m_gridPos.x && playerPos.y > m_gridPos.y)
				return true;
			if (targetPos.x == m_gridPos.x && targetPos.y > m_gridPos.y)
				return true;
		}
		return false;
	}

	void RockComponent::CheckEnemyCrush()
	{
		float cellSize = static_cast<float>(m_pGrid->GetCellSize());
		float rockCenterY = m_pixelY + cellSize * 0.5f;
		float crushThreshold = cellSize * 1.0f; // full cell of overlap tolerance

		for (auto* pEnemy : m_enemies)
		{
			if (!pEnemy || pEnemy->IsMarkedForDestroy()) continue;

			auto* enemyComp = pEnemy->GetComponent<EnemyComponent>();
			if (!enemyComp || !enemyComp->IsAlive()) continue;

			auto* movement = pEnemy->GetComponent<GridMovementComponent>();
			if (!movement) continue;

			const auto& enemyGridPos = movement->GetGridPosition();
			const auto& enemyTargetPos = movement->GetTargetGridPosition();

			// Enemy must be in the same column (current or target position)
			if (enemyGridPos.x != m_gridPos.x && enemyTargetPos.x != m_gridPos.x)
				continue;

			// Check if enemy is at or below the rock and within crush distance
			auto* enemyTransform = pEnemy->GetComponent<TransformComponent>();
			if (!enemyTransform) continue;

			float enemyCenterY = enemyTransform->GetWorldPosition().y + cellSize * 0.5f;
			float dist = std::abs(rockCenterY - enemyCenterY);

			if (dist < crushThreshold)
			{
				enemyComp->Crush();
				++m_crushCount;
			}
		}
	}

	void RockComponent::CheckPlayerCrush()
	{
		float cellSize = static_cast<float>(m_pGrid->GetCellSize());
		float rockCenterY = m_pixelY + cellSize * 0.5f;
		float crushThreshold = cellSize * 0.6f;

		for (auto* pPlayer : m_players)
		{
			if (!pPlayer || pPlayer->IsMarkedForDestroy()) continue;

			auto* movement = pPlayer->GetComponent<GridMovementComponent>();
			if (!movement) continue;

			const auto& playerPos = movement->GetGridPosition();

			// Player must be in the same column
			if (playerPos.x != m_gridPos.x) continue;

			auto* playerTransform = pPlayer->GetComponent<TransformComponent>();
			if (!playerTransform) continue;

			float playerCenterY = playerTransform->GetWorldPosition().y + cellSize * 0.5f;

			// Rock can only crush things it's falling ONTO — player must be
			// at or below the rock, not above it.
			if (playerCenterY < rockCenterY) continue;

			float dist = playerCenterY - rockCenterY;

			if (dist < crushThreshold)
			{
				auto* collision = pPlayer->GetComponent<PlayerCollisionComponent>();
				if (collision && !collision->IsDead())
					collision->TriggerDeath();
			}
		}
	}
}
