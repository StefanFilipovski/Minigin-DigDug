#pragma once
#include "Component.h"
#include "Subject.h"
#include <glm/glm.hpp>
#include <vector>
#include <memory>

namespace dae
{
	class GridComponent;
	class GridMovementComponent;
	class SpriteAnimatorComponent;
	class TransformComponent;
	class Texture2D;

	enum class RockState
	{
		Idle,       // Sitting on solid ground
		Unstable,   // Cell below is dug, player is directly underneath — waiting
		Jittering,  // Player moved away, shaking before falling
		Falling,    // Dropping through tunnels, crushing enemies
		Breaking,   // Hit the ground, playing break animation
		Destroyed   // Done — marked for removal
	};

	class RockComponent final : public Component, public Subject
	{
	public:
		RockComponent(GameObject* pOwner, GridComponent* pGrid,
			int gridX, int gridY);

		void Update(float deltaTime) override;
		void Render() const override;

		void AddPlayer(GameObject* pPlayer);
		void AddEnemy(GameObject* pEnemy);
		void ClearEnemies();

		RockState GetState() const { return m_state; }
		const glm::ivec2& GetGridPosition() const { return m_gridPos; }
		int GetCrushCount() const { return m_crushCount; }

	private:
		void UpdateIdle();
		void UpdateUnstable();
		void UpdateJittering(float deltaTime);
		void UpdateFalling(float deltaTime);
		void UpdateBreaking(float deltaTime);

		bool IsPlayerDirectlyBelow() const;
		void CheckEnemyCrush();
		void CheckPlayerCrush();

		GridComponent* m_pGrid;
		TransformComponent* m_pTransform{ nullptr };
		bool m_cached{ false };

		glm::ivec2 m_gridPos;
		RockState m_state{ RockState::Idle };

		// Jitter
		float m_jitterTimer{ 0.f };
		float m_jitterDuration{ 1.0f };
		float m_jitterOffset{ 0.f };
		float m_jitterSpeed{ 30.f };

		// Falling
		float m_fallSpeed{ 200.f };
		float m_pixelY{ 0.f };
		int m_startFallRow{ 0 };
		float m_playerGraceTimer{ 0.f };
		static constexpr float PlayerGraceDuration{ 0.5f }; // seconds of immunity after fall starts

		// Breaking
		float m_breakTimer{ 0.f };
		float m_breakDuration{ 0.6f };
		int m_breakFrame{ 0 };

		// Sprite
		std::shared_ptr<Texture2D> m_texture;
		static constexpr int SpriteW{ 16 };
		static constexpr int SpriteH{ 15 };

		// Rock crush scoring
		int m_crushCount{ 0 };

		// References
		std::vector<GameObject*> m_players;
		std::vector<GameObject*> m_enemies;

		void CacheComponents();
	};
}
