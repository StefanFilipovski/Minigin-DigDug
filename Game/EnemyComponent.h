#pragma once
#include "Component.h"
#include "Subject.h"
#include "EnemyStates.h"
#include "GameEventIds.h"
#include <glm/glm.hpp>
#include <string>
#include <memory>
#include <vector>
#include <SDL3/SDL.h>

namespace dae
{
	class GridComponent;
	class GridMovementComponent;
	class SpriteAnimatorComponent;
	class RenderComponent;
	class Texture2D;
	class PlayerCollisionComponent;
	struct EnemyTypeInfo;

	enum class EnemyType
	{
		Pooka,
		Fygar
	};

	// Per-enemy tuning values, grouped so state classes read plain data
	// instead of going through one trivial accessor per value
	struct EnemyTuning
	{
		float deflateTime{ 1.5f };
		float minFireInterval{ 4.f };
		float maxFireInterval{ 10.f };
		static constexpr int MaxInflateStages{ 5 };
	};

	class EnemyComponent final : public Component, public Subject
	{
	public:
		EnemyComponent(GameObject* pOwner, GridComponent* pGrid,
			EnemyType type, GameObject* pTarget);

		void AddTarget(GameObject* pTarget);
		void ClearTargets();

		// Player-controlled mode (Versus Fygar)
		void SetPlayerControlled(bool controlled);
		bool IsPlayerControlled() const { return m_playerControlled; }
		void StartFireBreath();

		// Player-controlled ghost form (Versus Fygar)
		void StartGhost();
		void StartGhostCooldown() { m_ghostCooldownRemaining = GhostCooldownDuration; }
		bool IsGhostReady() const { return m_ghostCooldownRemaining <= 0.f; }
		float GetGhostCooldownRemaining() const { return m_ghostCooldownRemaining; }

		void Update(float deltaTime) override;
		void FixedUpdate(float fixedTimeStep) override;
		void Render() const override;

		EnemyType GetEnemyType() const { return m_type; }
		EnemyStateType GetStateType() const;

		// External triggers for state changes
		void StartInflating(const glm::ivec2& attackDir = { -1, 0 });
		void PumpOnce();
		void Crush();

		// State queries 
		bool IsAlive() const;
		bool IsInflating() const;
		bool IsGhost() const;
		bool IsFireBreathing() const;
		bool IsPositionInFire(const glm::ivec2& pos) const;
		bool IsCollidingWith(GameObject* pOther) const;

		const glm::ivec2& GetGridPosition() const;
		int GetScoreValue() const;

		// Accessors for state classes
		GridMovementComponent* GetMovement() const;
		SpriteAnimatorComponent* GetAnimator() const;
		GridComponent* GetGrid() const { return m_pGrid; }
		GameObject* GetTarget() const;

		
		const std::string& GetLastHorizontalAnim() const { return m_lastHorizontalAnim; }
		void SetLastHorizontalAnim(const std::string& anim) { m_lastHorizontalAnim = anim; }

		const EnemyTuning& Tuning() const { return m_tuning; }

		// Scoring helpers
		const glm::ivec2& GetLastAttackDirection() const { return m_lastAttackDirection; }
		bool WasCrushedByRock() const { return m_crushedByRock; }

		void ChangeState(std::unique_ptr<EnemyState> newState);

		// Public wrapper so state classes can notify observers
		void Notify(EventId event, GameObject* pGameObject);

	private:
		GridComponent* m_pGrid;
		mutable GridMovementComponent* m_pMovement{ nullptr };
		mutable SpriteAnimatorComponent* m_pAnimator{ nullptr };
		mutable bool m_cached{ false };

		EnemyType m_type;
		const EnemyTypeInfo* m_pTypeInfo; 
		GameObject* m_pTarget; 

		// Target with its components resolved once at AddTarget time,
		// so the per-frame closest-player search does no GetComponent calls
		struct TargetEntry
		{
			GameObject* pObject{ nullptr };
			PlayerCollisionComponent* pCollision{ nullptr };
			GridMovementComponent* pMovement{ nullptr };
		};
		std::vector<TargetEntry> m_targets;

		std::unique_ptr<EnemyState> m_pCurrentState;
		bool m_stateInitialized{ false };

		EnemyTuning m_tuning{};

		std::string m_lastHorizontalAnim{ "walk_right" };

		glm::ivec2 m_lastAttackDirection{ 0, 0 };
		bool m_crushedByRock{ false };
		bool m_playerControlled{ false };

		// Ghost-form cooldown for the player-controlled Versus Fygar
		static constexpr float GhostCooldownDuration{ 4.f };
		float m_ghostCooldownRemaining{ 0.f };

		// Fire rendering 
		std::shared_ptr<Texture2D> m_fireTexture;

		void CacheComponents() const;
		void RenderFire() const;
	};
}
