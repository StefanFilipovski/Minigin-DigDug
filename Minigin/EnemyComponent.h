#pragma once
#include "Component.h"
#include "Subject.h"
#include <glm/glm.hpp>

namespace dae
{
	class GridComponent;
	class GridMovementComponent;
	class SpriteAnimatorComponent;
	class RenderComponent;

	enum class EnemyType
	{
		Pooka,
		Fygar
	};

	enum class EnemyState
	{
		Normal,      // Moving through tunnels, chasing player
		Ghost,       // Passing through dirt toward player
		Inflating,   // Being pumped by player (4 stages)
		Popped,      // Fully inflated — about to be destroyed
		Crushed      // Hit by a rock
	};

	class EnemyComponent final : public Component, public Subject
	{
	public:
		EnemyComponent(GameObject* pOwner, GridComponent* pGrid,
			EnemyType type, GameObject* pTarget);

		void Update(float deltaTime) override;
		void FixedUpdate(float fixedTimeStep) override;

		EnemyType GetEnemyType() const { return m_type; }
		EnemyState GetState() const { return m_state; }

		void StartInflating();
		void PumpOnce();
		bool IsInflating() const { return m_state == EnemyState::Inflating; }

		void Crush();

		bool IsCollidingWith(GameObject* pOther) const;

		const glm::ivec2& GetGridPosition() const;

		int GetScoreValue() const;

		bool IsAlive() const;

	private:
		void UpdateNormal(float deltaTime);
		void UpdateGhost(float deltaTime);
		void UpdateInflating(float deltaTime);

		glm::ivec2 ChooseDirection() const;
		glm::ivec2 ChooseGhostDirection() const;

		void EnterGhostMode();
		void ExitGhostMode();
		bool ShouldBecomeGhost() const;
		bool IsInTunnel() const;

		GridComponent* m_pGrid;
		mutable GridMovementComponent* m_pMovement{ nullptr };
		mutable SpriteAnimatorComponent* m_pAnimator{ nullptr };
		mutable bool m_cached{ false };

		EnemyType m_type;
		EnemyState m_state{ EnemyState::Normal };
		GameObject* m_pTarget; // The player to chase

		// Ghost mode
		float m_ghostTimer{ 0.f };
		float m_ghostCooldown{ 0.f };
		float m_minGhostInterval{ 5.f };   // Min seconds between ghost attempts
		float m_maxGhostInterval{ 12.f };   // Max seconds
		float m_ghostDuration{ 8.f };       // Max time in ghost mode

		// Inflating
		int m_inflateStage{ 0 };            // 0-3, pops at 4
		static constexpr int MaxInflateStages{ 4 };
		float m_deflateTimer{ 0.f };
		float m_deflateTime{ 1.5f };        // Seconds before deflating one stage

		// AI direction change
		float m_dirChangeTimer{ 0.f };
		float m_dirChangeInterval{ 0.3f };  // How often to reconsider direction

		void CacheComponents() const;
	};
}