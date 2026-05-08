#pragma once
#include "Component.h"
#include "Subject.h"
#include "EnemyStates.h"
#include "EventIds.h"
#include <glm/glm.hpp>
#include <string>
#include <memory>
#include <SDL3/SDL.h>

namespace dae
{
	class GridComponent;
	class GridMovementComponent;
	class SpriteAnimatorComponent;
	class RenderComponent;
	class Texture2D;

	enum class EnemyType
	{
		Pooka,
		Fygar
	};

	class EnemyComponent final : public Component, public Subject
	{
	public:
		EnemyComponent(GameObject* pOwner, GridComponent* pGrid,
			EnemyType type, GameObject* pTarget);

		void Update(float deltaTime) override;
		void FixedUpdate(float fixedTimeStep) override;
		void Render() const override;

		EnemyType GetEnemyType() const { return m_type; }
		EnemyStateType GetStateType() const;

		void StartInflating(const glm::ivec2& attackDir = { -1, 0 });
		void PumpOnce();
		bool IsInflating() const;
		bool IsGhost() const;
		bool IsFireBreathing() const;

		// Returns true if the given grid position is in this enemy's fire breath path
		bool IsPositionInFire(const glm::ivec2& pos) const;

		void Crush();

		bool IsCollidingWith(GameObject* pOther) const;

		const glm::ivec2& GetGridPosition() const;

		int GetScoreValue() const;

		bool IsAlive() const;

		// --- Accessors used by state classes ---
		GridMovementComponent* GetMovement() const;
		SpriteAnimatorComponent* GetAnimator() const;
		GridComponent* GetGrid() const { return m_pGrid; }
		GameObject* GetTarget() const { return m_pTarget; }

		const std::string& GetLastHorizontalAnim() const { return m_lastHorizontalAnim; }
		void SetLastHorizontalAnim(const std::string& anim) { m_lastHorizontalAnim = anim; }

		glm::ivec2 ChooseDirection() const;
		glm::ivec2 ChooseGhostDirection() const;
		bool ShouldBecomeGhost() const;
		bool IsInTunnel() const;

		float GetGhostDuration() const { return m_ghostDuration; }
		float GetMinGhostInterval() const { return m_minGhostInterval; }
		float GetMaxGhostInterval() const { return m_maxGhostInterval; }
		float GetDeflateTime() const { return m_deflateTime; }
		int GetMaxInflateStages() const { return MaxInflateStages; }
		float GetMinFireInterval() const { return m_minFireInterval; }
		float GetMaxFireInterval() const { return m_maxFireInterval; }

		// Persistent ghost cooldown so fire breathing doesnt reset progress
		float GetGhostCooldownRemaining() const { return m_ghostCooldownRemaining; }
		void SetGhostCooldownRemaining(float t) { m_ghostCooldownRemaining = t; }
		void RandomizeGhostCooldown();

		void ChangeState(std::unique_ptr<EnemyState> newState);

		// Public wrapper so state classes can notify observers
		void Notify(EventId event, GameObject* pGameObject);


	private:
		GridComponent* m_pGrid;
		mutable GridMovementComponent* m_pMovement{ nullptr };
		mutable SpriteAnimatorComponent* m_pAnimator{ nullptr };
		mutable bool m_cached{ false };

		EnemyType m_type;
		GameObject* m_pTarget;

		std::unique_ptr<EnemyState> m_pCurrentState;
		bool m_stateInitialized{ false };

		static constexpr int MaxInflateStages{ 5 };

		// Settings
		float m_minGhostInterval{ 5.f };
		float m_maxGhostInterval{ 12.f };
		float m_ghostDuration{ 8.f };
		float m_deflateTime{ 1.5f };
		float m_minFireInterval{ 4.f };
		float m_maxFireInterval{ 10.f };

		std::string m_lastHorizontalAnim{ "walk_right" };
		float m_ghostCooldownRemaining{ -1.f }; // -1 means not initialized

		// Fire rendering (Fygar only)
		std::shared_ptr<Texture2D> m_fireTexture;

		void CacheComponents() const;
		void RenderFire() const;
	};
}
