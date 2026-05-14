#pragma once
#include "Component.h"
#include "Subject.h"
#include <vector>
#include <glm/glm.hpp>

namespace dae
{
	class GridMovementComponent;
	class GridComponent;

	class PlayerCollisionComponent final : public Component, public Subject
	{
	public:
		PlayerCollisionComponent(GameObject* pOwner, GridComponent* pGrid);

		void Update(float deltaTime) override;

		void AddEnemy(GameObject* pEnemy);
		void ClearEnemies();

		int GetLives() const { return m_lives; }
		void SetLives(int lives) { m_lives = lives; }
		void SetSpawnPos(const glm::ivec2& pos) { m_spawnPos = pos; }

		bool IsDead() const { return m_dead; }
		void Respawn(int gridX, int gridY);
		void TriggerDeath();

	private:
		GridComponent* m_pGrid;
		GridMovementComponent* m_pMovement{ nullptr };
		bool m_cached{ false };

		std::vector<GameObject*> m_enemies;
		int m_lives{ 4 }; // 1 starting + 3 extra
		bool m_dead{ false };
		float m_respawnTimer{ 0.f };
		float m_respawnDelay{ 1.5f };

		glm::ivec2 m_spawnPos{ 0, 0 };
	};
}