#include "EnemyComponent.h"
#include "EnemyTypeInfo.h"
#include "EnemyStates.h"
#include "GridComponent.h"
#include "GridMovementComponent.h"
#include "SpriteAnimatorComponent.h"
#include "RenderComponent.h"
#include "TransformComponent.h"
#include "ResourceManager.h"
#include "Renderer.h"
#include "Texture2D.h"
#include "PlayerCollisionComponent.h"
#include "GameEventIds.h"
#include "GameObject.h"
#include <string>
#include <climits>

namespace dae
{
	EnemyComponent::EnemyComponent(GameObject* pOwner, GridComponent* pGrid,
		EnemyType type, GameObject* pTarget)
		: Component(pOwner)
		, m_pGrid(pGrid)
		, m_type(type)
		, m_pTypeInfo(&GetEnemyTypeInfo(type,
			pGrid ? static_cast<float>(pGrid->GetCellSize()) : 32.f))
		, m_pTarget(pTarget)
	{
		if (pTarget)
			AddTarget(pTarget);

		m_pCurrentState = std::make_unique<EnemyNormalState>();

		if (!m_pTypeInfo->fireTexture.empty())
		{
			m_fireTexture = ResourceManager::GetInstance().LoadTexture(
				m_pTypeInfo->fireTexture, SpriteSheetColorKey);
		}
	}

	void EnemyComponent::AddTarget(GameObject* pTarget)
	{
		if (!pTarget) return;

		m_targets.push_back({
			pTarget,
			pTarget->GetComponent<PlayerCollisionComponent>(),
			pTarget->GetComponent<GridMovementComponent>() });
	}

	void EnemyComponent::ClearTargets()
	{
		m_targets.clear();
		m_pTarget = nullptr;
	}

	GameObject* EnemyComponent::GetTarget() const
	{
		CacheComponents();

		// If only one target, use it directly
		if (m_targets.size() <= 1)
			return m_pTarget;

		// Find the closest living player
		GameObject* closest = nullptr;
		int bestDistSq = INT_MAX;

		const auto& myPos = GetGridPosition();

		for (const auto& target : m_targets)
		{
			if (!target.pObject || target.pObject->IsMarkedForDestroy()) continue;

			// Skip dead players — they're in the death animation
			if (target.pCollision && target.pCollision->IsDead()) continue;

			if (!target.pMovement) continue;

			const auto& targetPos = target.pMovement->GetGridPosition();
			glm::ivec2 diff = targetPos - myPos;
			int distSq = diff.x * diff.x + diff.y * diff.y;

			if (distSq < bestDistSq)
			{
				bestDistSq = distSq;
				closest = target.pObject;
			}
		}

		// Fall back to primary target if no living player found
		return closest ? closest : m_pTarget;
	}

	void EnemyComponent::CacheComponents() const
	{
		if (m_cached) return;
		m_pMovement = GetOwner()->GetComponent<GridMovementComponent>();
		m_pAnimator = GetOwner()->GetComponent<SpriteAnimatorComponent>();
		m_cached = true;
	}

	void EnemyComponent::Update(float deltaTime)
	{
		CacheComponents();
		if (!m_pMovement) return;

		// Tick the ghost-form cooldown (player-controlled Versus Fygar)
		if (m_ghostCooldownRemaining > 0.f)
			m_ghostCooldownRemaining -= deltaTime;

		// Call Enter on the initial state once components are ready
		if (!m_stateInitialized && m_pCurrentState)
		{
			m_pCurrentState->Enter(*this);
			m_stateInitialized = true;
		}

		if (m_pCurrentState)
		{
			auto newState = m_pCurrentState->Update(*this, deltaTime);
			if (newState)
				ChangeState(std::move(newState));
		}
	}

	void EnemyComponent::FixedUpdate(float /*fixedTimeStep*/)
	{
	}

	void EnemyComponent::Notify(EventId event, GameObject* pGameObject)
	{
		NotifyObservers(event, pGameObject);
	}

	void EnemyComponent::ChangeState(std::unique_ptr<EnemyState> newState)
	{
		if (m_pCurrentState)
			m_pCurrentState->Exit(*this);

		m_pCurrentState = std::move(newState);

		if (m_pCurrentState)
			m_pCurrentState->Enter(*this);
	}

	EnemyStateType EnemyComponent::GetStateType() const
	{
		if (m_pCurrentState)
			return m_pCurrentState->GetType();
		return EnemyStateType::Normal;
	}

	bool EnemyComponent::IsAlive() const
	{
		auto type = GetStateType();
		return type != EnemyStateType::Popped && type != EnemyStateType::Crushed;
	}

	bool EnemyComponent::IsInflating() const
	{
		return GetStateType() == EnemyStateType::Inflating;
	}

	bool EnemyComponent::IsGhost() const
	{
		return GetStateType() == EnemyStateType::Ghost;
	}

	bool EnemyComponent::IsFireBreathing() const
	{
		return GetStateType() == EnemyStateType::FireBreathing;
	}

	bool EnemyComponent::IsPositionInFire(const glm::ivec2& pos) const
	{
		if (!IsFireBreathing()) return false;

		auto* fireState = dynamic_cast<const EnemyFireBreathingState*>(m_pCurrentState.get());
		if (!fireState) return false;

		const auto& myPos = GetGridPosition();
		const glm::ivec2& fireDir = fireState->GetFireDirection();
		int range = fireState->GetCurrentFireRange();

		for (int i = 1; i <= range; ++i)
		{
			if (myPos + fireDir * i == pos)
				return true;
		}
		return false;
	}

	void EnemyComponent::Render() const
	{
		if (IsFireBreathing())
			RenderFire();
	}

	void EnemyComponent::RenderFire() const
	{
		if (!m_fireTexture) return;

		auto* fireState = dynamic_cast<const EnemyFireBreathingState*>(m_pCurrentState.get());
		if (!fireState || fireState->GetCurrentFireRange() <= 0) return;

		auto* transform = GetOwner()->GetComponent<TransformComponent>();
		if (!transform || !m_pGrid) return;

		const auto& pos = transform->GetWorldPosition();
		float cs = static_cast<float>(m_pGrid->GetCellSize());
		const glm::ivec2& fireDir = fireState->GetFireDirection();
		int range = fireState->GetCurrentFireRange();

		auto* sdlRenderer = Renderer::GetInstance().GetSDLRenderer();
		auto* sdlTexture = m_fireTexture->GetSDLTexture();

		bool facingLeft = (fireDir.x < 0);

		for (int i = 1; i <= range; ++i)
		{
			SDL_FRect src;
			float dstW;

			if (i == 1)
			{
				src = { 0.f, 0.f, 16.f, 16.f };
				dstW = cs;
			}
			else
			{
				src = { 16.f, 0.f, 32.f, 16.f };
				dstW = cs;
			}

			float fireX = pos.x + static_cast<float>(fireDir.x * i) * cs;
			float fireY = pos.y;

			SDL_FRect dst{ fireX, fireY, dstW, cs };
			SDL_FlipMode flip = facingLeft ? SDL_FLIP_HORIZONTAL : SDL_FLIP_NONE;
			SDL_RenderTextureRotated(sdlRenderer, sdlTexture, &src, &dst, 0.0, nullptr, flip);
		}
	}

	// External triggers 

	void EnemyComponent::SetPlayerControlled(bool controlled)
	{
		m_playerControlled = controlled;
		if (controlled)
		{
			// Switch to idle state
			ChangeState(std::make_unique<EnemyIdleState>());
			m_stateInitialized = true;
		}
	}

	void EnemyComponent::StartFireBreath()
	{
		if (m_pTypeInfo->fireTexture.empty()) return; 
		if (!IsAlive() || IsInflating() || IsFireBreathing()) return;

		// Can't breathe fire while phasing through the ground
		if (IsGhost()) return;

		ChangeState(std::make_unique<EnemyFireBreathingState>());
	}

	void EnemyComponent::StartGhost()
	{
		if (!m_playerControlled) return;
		// Only from the idle (normal player-controlled) state, and not on cooldown
		if (GetStateType() != EnemyStateType::Idle) return;
		if (m_ghostCooldownRemaining > 0.f) return;

		ChangeState(std::make_unique<EnemyPlayerGhostState>());
	}

	void EnemyComponent::StartInflating(const glm::ivec2& attackDir)
	{
		auto type = GetStateType();
		if (type == EnemyStateType::Popped || type == EnemyStateType::Crushed)
			return;
		if (type == EnemyStateType::Ghost || type == EnemyStateType::FireBreathing)
			return;

		m_lastAttackDirection = attackDir;
		ChangeState(std::make_unique<EnemyInflatingState>(attackDir));
	}

	void EnemyComponent::PumpOnce()
	{
		if (!IsInflating()) return;

		auto* inflating = dynamic_cast<EnemyInflatingState*>(m_pCurrentState.get());
		if (inflating)
		{
			auto newState = inflating->PumpOnce(*this);
			if (newState)
				ChangeState(std::move(newState));
		}
	}

	void EnemyComponent::Crush()
	{
		m_crushedByRock = true;
		ChangeState(std::make_unique<EnemyCrushedState>());
	}

	bool EnemyComponent::IsCollidingWith(GameObject* pOther) const
	{
		if (!IsAlive()) return false;

		if (IsInflating())
		{
			auto* inflating = dynamic_cast<const EnemyInflatingState*>(m_pCurrentState.get());
			if (inflating && inflating->GetInflateStage() > 0)
				return false;
		}

		CacheComponents();
		if (!m_pMovement) return false;

		auto* otherMovement = pOther->GetComponent<GridMovementComponent>();
		if (!otherMovement) return false;

		return m_pMovement->GetGridPosition() == otherMovement->GetGridPosition();
	}

	const glm::ivec2& EnemyComponent::GetGridPosition() const
	{
		static glm::ivec2 fallback{ 0, 0 };
		CacheComponents();
		if (m_pMovement) return m_pMovement->GetGridPosition();
		return fallback;
	}

	int EnemyComponent::GetScoreValue() const
	{
		CacheComponents();
		if (!m_pMovement || !m_pGrid) return 200;

		int layer = m_pGrid->GetLayer(m_pMovement->GetGridPosition().y);

		// Base score by layer (same for Pooka and Fygar base).
		// Fygar horizontal bonus (2x) is applied by the score system.
		int base = 200;
		switch (layer)
		{
		case 1: base = 200; break;
		case 2: base = 300; break;
		case 3: base = 400; break;
		case 4: base = 500; break;
		default: base = 200; break;
		}

		if (m_pTypeInfo->horizontalKillBonus && m_lastAttackDirection.x != 0)
			base *= 2;

		return base;
	}

	GridMovementComponent* EnemyComponent::GetMovement() const
	{
		CacheComponents();
		return m_pMovement;
	}

	SpriteAnimatorComponent* EnemyComponent::GetAnimator() const
	{
		CacheComponents();
		return m_pAnimator;
	}
}
