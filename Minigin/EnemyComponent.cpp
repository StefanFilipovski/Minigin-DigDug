#include "EnemyComponent.h"
#include "GridComponent.h"
#include "GridMovementComponent.h"
#include "SpriteAnimatorComponent.h"
#include "RenderComponent.h"
#include "TransformComponent.h"
#include "EventIds.h"
#include "GameObject.h"
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
		// Randomize initial ghost cooldown so enemies don't all ghost at once
		m_ghostCooldown = m_minGhostInterval +
			static_cast<float>(std::rand() % 100) / 100.f * (m_maxGhostInterval - m_minGhostInterval);
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

		switch (m_state)
		{
		case EnemyState::Normal:
			UpdateNormal(deltaTime);
			break;
		case EnemyState::Ghost:
			UpdateGhost(deltaTime);
			break;
		case EnemyState::Inflating:
			UpdateInflating(deltaTime);
			break;
		case EnemyState::Popped:
		case EnemyState::Crushed:
			// Dead — will be cleaned up
			break;
		}
	}

	void EnemyComponent::FixedUpdate(float /*fixedTimeStep*/)
	{
		// Direction is set in Update; GridMovementComponent handles the actual movement
	}

	void EnemyComponent::UpdateNormal(float deltaTime)
	{
		m_dirChangeTimer += deltaTime;
		if (m_dirChangeTimer >= m_dirChangeInterval)
		{
			m_dirChangeTimer = 0.f;

			if (!m_pMovement->IsMoving())
			{
				glm::ivec2 dir = ChooseDirection();
				m_pMovement->SetDesiredDirection(dir);

				if (m_pAnimator)
				{
					if (dir.x > 0) m_pAnimator->Play("walk_right");
					else if (dir.x < 0) m_pAnimator->Play("walk_left");
					else if (dir.y < 0) m_pAnimator->Play("walk_up");
					else if (dir.y > 0) m_pAnimator->Play("walk_down");
					m_pAnimator->Resume();
				}
			}
		}

		// Keep re-setting direction while mid-move so the enemy doesn't coast to a stop
		if (m_pMovement->IsMoving())
		{
			m_pMovement->SetDesiredDirection(m_pMovement->GetCurrentDirection());
		}

		m_ghostCooldown -= deltaTime;
		if (m_ghostCooldown <= 0.f && ShouldBecomeGhost())
		{
			EnterGhostMode();
		}
	}

	void EnemyComponent::UpdateGhost(float deltaTime)
	{
		m_ghostTimer += deltaTime;

		if (m_ghostTimer >= m_ghostDuration)
		{
			if (IsInTunnel())
			{
				ExitGhostMode();
				return;
			}
			// Keep going until we find a tunnel to re-enter
		}

		// Small delay avoids immediately exiting the moment we entered ghost mode
		if (IsInTunnel() && m_ghostTimer > 1.f)
		{
			ExitGhostMode();
			return;
		}

		m_dirChangeTimer += deltaTime;
		if (m_dirChangeTimer >= m_dirChangeInterval)
		{
			m_dirChangeTimer = 0.f;

			if (!m_pMovement->IsMoving())
			{
				glm::ivec2 dir = ChooseGhostDirection();
				m_pMovement->SetDesiredDirection(dir);
			}
		}

		if (m_pMovement->IsMoving())
		{
			m_pMovement->SetDesiredDirection(m_pMovement->GetCurrentDirection());
		}
	}

	void EnemyComponent::UpdateInflating(float deltaTime)
	{
		// Deflates one stage per m_deflateTime seconds if the player stops pumping
		m_deflateTimer += deltaTime;
		if (m_deflateTimer >= m_deflateTime)
		{
			m_deflateTimer = 0.f;
			--m_inflateStage;

			if (m_inflateStage <= 0)
			{
				m_inflateStage = 0;
				m_state = EnemyState::Normal;

				if (m_pAnimator)
					m_pAnimator->Play("walk_right");

				return;
			}

			if (m_pAnimator)
			{
				std::string animName = "inflate_" + std::to_string(m_inflateStage);
				m_pAnimator->Play(animName);
			}
		}
	}

	void EnemyComponent::StartInflating()
	{
		if (m_state == EnemyState::Popped || m_state == EnemyState::Crushed)
			return;

		m_state = EnemyState::Inflating;
		m_inflateStage = 1;
		m_deflateTimer = 0.f;

		m_pMovement->SetDesiredDirection({ 0, 0 });

		if (m_pAnimator)
			m_pAnimator->Play("inflate_1");
	}

	void EnemyComponent::PumpOnce()
	{
		if (m_state != EnemyState::Inflating) return;

		m_deflateTimer = 0.f; // Resets the deflate countdown on each pump press
		++m_inflateStage;

		if (m_inflateStage >= MaxInflateStages)
		{
			m_state = EnemyState::Popped;
			NotifyObservers(EVENT_ENEMY_KILLED, GetOwner());
			GetOwner()->MarkForDestroy();
			return;
		}

		if (m_pAnimator)
		{
			std::string animName = "inflate_" + std::to_string(m_inflateStage);
			m_pAnimator->Play(animName);
		}
	}

	void EnemyComponent::Crush()
	{
		m_state = EnemyState::Crushed;
		NotifyObservers(EVENT_ENEMY_KILLED, GetOwner());
		GetOwner()->MarkForDestroy();
	}

	bool EnemyComponent::IsCollidingWith(GameObject* pOther) const
	{
		if (!IsAlive()) return false;
		if (m_state == EnemyState::Inflating && m_inflateStage > 0) return false; // Player can pass through inflating enemies

		CacheComponents();
		if (!m_pMovement) return false;

		auto* otherMovement = pOther->GetComponent<GridMovementComponent>();
		if (!otherMovement) return false;

		return m_pMovement->GetGridPosition() == otherMovement->GetGridPosition();
	}

	const glm::ivec2& EnemyComponent::GetGridPosition() const
	{
		static glm::ivec2 fallback{ 0, 0 };
		if (m_pMovement) return m_pMovement->GetGridPosition();
		return fallback;
	}

	int EnemyComponent::GetScoreValue() const
	{
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
		else // Fygar — base values (horizontal kill doubles, handled elsewhere)
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

	bool EnemyComponent::IsAlive() const
	{
		return m_state != EnemyState::Popped && m_state != EnemyState::Crushed;
	}

	glm::ivec2 EnemyComponent::ChooseDirection() const
	{
		if (!m_pTarget || !m_pMovement || !m_pGrid) return { 1, 0 };

		auto* targetMovement = m_pTarget->GetComponent<GridMovementComponent>();
		if (!targetMovement) return { 1, 0 };

		const auto& myPos = m_pMovement->GetGridPosition();
		const auto& targetPos = targetMovement->GetGridPosition();
		glm::ivec2 diff = targetPos - myPos;

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

			// Distance to player if we go this way
			glm::ivec2 newDiff = targetPos - target;
			opt.distSq = newDiff.x * newDiff.x + newDiff.y * newDiff.y;
		}

		std::sort(std::begin(options), std::end(options),
			[](const DirOption& a, const DirOption& b)
			{
				return a.distSq < b.distSq;
			});

		// Prefer directions that get us closer, avoid reversing unless it's the only option
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

		// Prefer the axis with the largest remaining distance
		if (std::abs(diff.x) >= std::abs(diff.y))
		{
			if (diff.x > 0) return { 1, 0 };
			if (diff.x < 0) return { -1, 0 };
		}

		if (diff.y > 0) return { 0, 1 };
		if (diff.y < 0) return { 0, -1 };

		return { 1, 0 };
	}

	void EnemyComponent::EnterGhostMode()
	{
		m_state = EnemyState::Ghost;
		m_ghostTimer = 0.f;
		m_pMovement->SetGhostMode(true);

		if (m_pAnimator)
			m_pAnimator->Play("ghost");
	}

	void EnemyComponent::ExitGhostMode()
	{
		m_state = EnemyState::Normal;
		m_ghostTimer = 0.f;
		m_ghostCooldown = m_minGhostInterval +
			static_cast<float>(std::rand() % 100) / 100.f * (m_maxGhostInterval - m_minGhostInterval);

		m_pMovement->SetGhostMode(false);

		if (m_pAnimator)
			m_pAnimator->Play("walk_right");
	}

	bool EnemyComponent::ShouldBecomeGhost() const
	{
		if (!m_pMovement || !m_pTarget) return false;

		// Only become ghost if we can't reach the player through tunnels easily
		// Simple heuristic: if stuck (no passable direction gets us closer), ghost
		auto* targetMovement = m_pTarget->GetComponent<GridMovementComponent>();
		if (!targetMovement) return false;

		const auto& myPos = m_pMovement->GetGridPosition();
		const auto& targetPos = targetMovement->GetGridPosition();

		// Manhattan distance
		int dist = std::abs(targetPos.x - myPos.x) + std::abs(targetPos.y - myPos.y);

		// More likely to ghost when far from player
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