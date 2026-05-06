#include "EnemyComponent.h"
#include "EnemyStates.h"
#include "GridComponent.h"
#include "GridMovementComponent.h"
#include "SpriteAnimatorComponent.h"
#include "RenderComponent.h"
#include "TransformComponent.h"
#include "EventIds.h"
#include "GameObject.h"
#include <string>
#include <cstdlib>
#include <cmath>
#include <algorithm>

namespace dae
{
	EnemyComponent::EnemyComponent(GameObject* pOwner, GridComponent* pGrid,
		EnemyType type, GameObject* pTarget)
		: Component(pOwner)
		, m_pGrid(pGrid)
		, m_type(type)
		, m_pTarget(pTarget)
	{
		auto initialState = std::make_unique<EnemyNormalState>();
		m_pCurrentState = std::move(initialState);
		// Enter is called after components are ready (first Update)
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

		// Call Enter on the initial state once components are ready
		if (!m_stateInitialized && m_pCurrentState)
		{
			m_pCurrentState->Enter(*this);
			m_stateInitialized = true;
		}

		if (m_pCurrentState)
			m_pCurrentState->Update(*this, deltaTime);
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

	// --- Public API (delegates to current state where needed) ---

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

	void EnemyComponent::StartInflating(const glm::ivec2& attackDir)
	{
		auto type = GetStateType();
		if (type == EnemyStateType::Popped || type == EnemyStateType::Crushed)
			return;

		ChangeState(std::make_unique<EnemyInflatingState>(attackDir));
	}

	void EnemyComponent::PumpOnce()
	{
		if (!IsInflating()) return;

		auto* inflating = dynamic_cast<EnemyInflatingState*>(m_pCurrentState.get());
		if (inflating)
			inflating->PumpOnce(*this);
	}

	void EnemyComponent::Crush()
	{
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

		if (m_type == EnemyType::Pooka)
		{
			switch (layer)
			{
			case 1: return 200;
			case 2: return 300;
			case 3: return 400;
			case 4: return 500;
			default: return 200;
			}
		}
		else
		{
			switch (layer)
			{
			case 1: return 400;
			case 2: return 600;
			case 3: return 800;
			case 4: return 1000;
			default: return 400;
			}
		}
	}

	// --- Accessors for state classes ---

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

	// --- AI helpers (used by NormalState and GhostState) ---

	glm::ivec2 EnemyComponent::ChooseDirection() const
	{
		if (!m_pTarget || !m_pMovement || !m_pGrid) return { 1, 0 };

		auto* targetMovement = m_pTarget->GetComponent<GridMovementComponent>();
		if (!targetMovement) return { 1, 0 };

		const auto& myPos = m_pMovement->GetGridPosition();
		const auto& targetPos = targetMovement->GetGridPosition();

		struct DirOption
		{
			glm::ivec2 dir;
			int distSq;
			bool passable;
		};

		DirOption options[4] = {
			{{ 1, 0}, 0, false},
			{{-1, 0}, 0, false},
			{{ 0,-1}, 0, false},
			{{ 0, 1}, 0, false}
		};

		glm::ivec2 currentDir = m_pMovement->GetCurrentDirection();

		for (auto& opt : options)
		{
			glm::ivec2 target = myPos + opt.dir;
			opt.passable = m_pGrid->IsInBounds(target.x, target.y) &&
				m_pGrid->GetCellType(target.x, target.y) != CellType::Dirt;

			glm::ivec2 newDiff = targetPos - target;
			opt.distSq = newDiff.x * newDiff.x + newDiff.y * newDiff.y;
		}

		std::sort(std::begin(options), std::end(options),
			[](const DirOption& a, const DirOption& b)
			{
				return a.distSq < b.distSq;
			});

		glm::ivec2 reverse = -currentDir;

		for (const auto& opt : options)
		{
			if (opt.passable && opt.dir != reverse)
				return opt.dir;
		}

		for (const auto& opt : options)
		{
			if (opt.passable)
				return opt.dir;
		}

		return { 0, 0 };
	}

	glm::ivec2 EnemyComponent::ChooseGhostDirection() const
	{
		if (!m_pTarget || !m_pMovement) return { 1, 0 };

		auto* targetMovement = m_pTarget->GetComponent<GridMovementComponent>();
		if (!targetMovement) return { 1, 0 };

		const auto& myPos = m_pMovement->GetGridPosition();
		const auto& targetPos = targetMovement->GetGridPosition();
		glm::ivec2 diff = targetPos - myPos;

		if (std::abs(diff.x) >= std::abs(diff.y))
		{
			if (diff.x > 0) return { 1, 0 };
			if (diff.x < 0) return { -1, 0 };
		}

		if (diff.y > 0) return { 0, 1 };
		if (diff.y < 0) return { 0, -1 };

		return { 1, 0 };
	}

	bool EnemyComponent::ShouldBecomeGhost() const
	{
		if (!m_pMovement || !m_pTarget) return false;

		auto* targetMovement = m_pTarget->GetComponent<GridMovementComponent>();
		if (!targetMovement) return false;

		const auto& myPos = m_pMovement->GetGridPosition();
		const auto& targetPos = targetMovement->GetGridPosition();

		int dist = std::abs(targetPos.x - myPos.x) + std::abs(targetPos.y - myPos.y);

		return dist > 5 || (std::rand() % 100 < 20);
	}

	bool EnemyComponent::IsInTunnel() const
	{
		if (!m_pMovement || !m_pGrid) return false;
		const auto& pos = m_pMovement->GetGridPosition();
		CellType type = m_pGrid->GetCellType(pos.x, pos.y);
		return type == CellType::Tunnel || type == CellType::Surface;
	}
}
