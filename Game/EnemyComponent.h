#pragma once
#include "Component.h"
#include "Subject.h"
#include "EnemyStates.h"
#include "GameEventIds.h"
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

		// External triggers for state changes
		void StartInflating(const glm::ivec2& attackDir = { -1, 0 });
		void PumpOnce();
		void Crush();

		// State queries used by other systems (collision, pump, etc.)
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
		GameObject* GetTarget() const { return m_pTarget; }

		const std::string& GetLastHorizontalAnim() const { return m_lastHorizontalAnim; }
		void SetLastHorizontalAnim(const std::string& anim) { m_lastHorizontalAnim = anim; }

		float GetDeflateTime() const { return m_deflateTime; }
		int GetMaxInflateStages() const { return MaxInflateStages; }
		float GetMinFireInterval() const { return m_minFireInterval; }
		float GetMaxFireInterval() const { return m_maxFireInterval; }

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
		float m_deflateTime{ 1.5f };
		float m_minFireInterval{ 4.f };
		float m_maxFireInterval{ 10.f };

		std::string m_lastHorizontalAnim{ "walk_right" };

		// Fire rendering (Fygar only)
		std::shared_ptr<Texture2D> m_fireTexture;

		void CacheComponents() const;
		void RenderFire() const;
	};
}
