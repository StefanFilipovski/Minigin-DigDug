#pragma once
#include "Component.h"
#include "Subject.h"
#include <vector>
#include <functional>
#include <glm/glm.hpp>

namespace dae
{
	class GridMovementComponent;
	class SpriteAnimatorComponent;
	class GridComponent;

	class PlayerCollisionComponent final : public Component, public Subject
	{
	public:
		PlayerCollisionComponent(GameObject* pOwner, GridComponent* pGrid);

		void Update(float deltaTime) override;

		bool IsInBlackScreen() const { return m_deathPhase == DeathPhase::BlackScreen; }

		void AddEnemy(GameObject* pEnemy);
		void ClearEnemies();

		int GetLives() const { return m_lives; }
		void SetLives(int lives) { m_lives = lives; }
		void SetSpawnPos(const glm::ivec2& pos) { m_spawnPos = pos; }

		bool IsDead() const { return m_dead; }
		void Respawn(int gridX, int gridY);
		void TriggerDeath();

		// Callback is called after black screen, before respawn completes.
		// The callback should re-create enemies and rebind input.

		// Callback for soft-reset (rebinds input, etc.)
		void SetSoftResetCallback(std::function<void()> cb) { m_softResetCallback = std::move(cb); }
		// Callback for game over (transition to game over state)
		void SetGameOverCallback(std::function<void()> cb) { m_gameOverCallback = std::move(cb); }

	private:
		enum class DeathPhase
		{
			None,
			DyingAnimation,
			BlackScreen,
			SoftReset
		};

		GridComponent* m_pGrid;
		GridMovementComponent* m_pMovement{ nullptr };
		SpriteAnimatorComponent* m_pAnimator{ nullptr };
		bool m_cached{ false };

		std::vector<GameObject*> m_enemies;

		int m_lives{ 4 };
		bool m_dead{ false };

		DeathPhase m_deathPhase{ DeathPhase::None };
		float m_phaseTimer{ 0.f };

		static constexpr float LastFrameHold{ 0.5f };  // show last death frame briefly
		static constexpr float BlackScreenDuration{ 1.0f };

		glm::ivec2 m_spawnPos{ 0, 0 };

		std::function<void()> m_softResetCallback;
		std::function<void()> m_gameOverCallback;

		void SoftReset();
	};
}