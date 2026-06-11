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

		// Soft-reset callback: re-creates enemies and rebinds input
		void SetSoftResetCallback(std::function<void()> cb) { m_softResetCallback = std::move(cb); }
		// Game-over callback: transitions to the game over state
		void SetGameOverCallback(std::function<void()> cb) { m_gameOverCallback = std::move(cb); }

		// Co-op shared-lives mode: only plays the death animation, then parks in
		// AwaitingRespawn for PlayingState to decide lives/respawn/game-over.
		void SetCoopShared(bool shared) { m_coopShared = shared; }
		void SetOnDeathStartCallback(std::function<void()> cb) { m_onDeathStartCallback = std::move(cb); }
		bool IsAwaitingRespawn() const { return m_deathPhase == DeathPhase::AwaitingRespawn; }

	private:
		enum class DeathPhase
		{
			None,
			DyingAnimation,
			BlackScreen,
			SoftReset,
			AwaitingRespawn   // co-op: death anim done, waiting for PlayingState
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
		std::function<void()> m_onDeathStartCallback;
		bool m_coopShared{ false };

		void SoftReset();
	};
}