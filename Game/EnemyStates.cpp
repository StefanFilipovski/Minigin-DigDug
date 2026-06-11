#include "EnemyStates.h"
#include "EnemyComponent.h"
#include "GridComponent.h"
#include "GridMovementComponent.h"
#include "SpriteAnimatorComponent.h"
#include "RenderComponent.h"
#include "ServiceLocator.h"
#include "GameSounds.h"
#include "GameEventIds.h"
#include "GameObject.h"
#include <cstdlib>
#include <cmath>
#include <string>
#include <algorithm>

namespace dae
{
	// IdleState (player-controlled)

	void EnemyIdleState::Enter(EnemyComponent& enemy)
	{
		auto* animator = enemy.GetAnimator();
		if (animator)
		{
			animator->Play(enemy.GetLastHorizontalAnim());
			animator->Resume();
		}
	}

	std::unique_ptr<EnemyState> EnemyIdleState::Update(EnemyComponent& enemy, float /*deltaTime*/)
	{
		// No AI — just update walk animation based on current movement direction
		auto* movement = enemy.GetMovement();
		auto* animator = enemy.GetAnimator();
		if (!movement || !animator) return nullptr;

		if (movement->IsMoving())
		{
			glm::ivec2 dir = movement->GetCurrentDirection();
			if (dir.x > 0)      { enemy.SetLastHorizontalAnim("walk_right"); animator->Play("walk_right"); }
			else if (dir.x < 0) { enemy.SetLastHorizontalAnim("walk_left");  animator->Play("walk_left"); }
			else if (dir.y < 0) animator->Play("walk_up");
			else if (dir.y > 0) animator->Play("walk_down");
			animator->Resume();
		}
		else
		{
			animator->Pause();
		}

		return nullptr;
	}

	void EnemyIdleState::Exit(EnemyComponent& /*enemy*/)
	{
	}

	// PlayerGhostState (player-controlled Fygar)

	void EnemyPlayerGhostState::Enter(EnemyComponent& enemy)
	{
		m_hasBeenInDirt = false;
		m_timer = 0.f;

		auto* movement = enemy.GetMovement();
		if (movement) movement->SetGhostMode(true);

		auto* animator = enemy.GetAnimator();
		if (animator)
		{
			animator->Play("ghost");
			animator->Resume();
		}
	}

	std::unique_ptr<EnemyState> EnemyPlayerGhostState::Update(EnemyComponent& enemy, float deltaTime)
	{
		m_timer += deltaTime;

		auto* movement = enemy.GetMovement();
		auto* grid = enemy.GetGrid();
		if (!movement || !grid) return nullptr;

		// Only evaluate exit conditions when settled on a cell, so the Fygar
		// never leaves ghost form while still inside dirt.
		if (movement->IsMoving()) return nullptr;

		const auto& pos = movement->GetGridPosition();
		CellType cell = grid->GetCellType(pos.x, pos.y);

		if (cell == CellType::Dirt)
		{
			m_hasBeenInDirt = true;
			return nullptr; // never exit while standing in dirt
		}

		// On an empty cell (Tunnel/Surface): exit once we've travelled through
		// dirt, or once the safety duration elapses.
		if (m_hasBeenInDirt || m_timer >= m_maxDuration)
			return std::make_unique<EnemyIdleState>();

		return nullptr;
	}

	void EnemyPlayerGhostState::Exit(EnemyComponent& enemy)
	{
		auto* movement = enemy.GetMovement();
		if (movement) movement->SetGhostMode(false);

		// Start the cooldown before ghost form can be used again
		enemy.StartGhostCooldown();
	}

	// NormalState

	EnemyNormalState::EnemyNormalState(float ghostCooldown)
		: m_ghostCooldown(ghostCooldown)
	{
	}

	void EnemyNormalState::Enter(EnemyComponent& enemy)
	{
		// Randomize fire cooldown for Fygar
		if (enemy.GetEnemyType() == EnemyType::Fygar)
		{
			const EnemyTuning& tuning = enemy.Tuning();
			m_fireCooldown = tuning.minFireInterval +
				static_cast<float>(std::rand() % 100) / 100.f *
				(tuning.maxFireInterval - tuning.minFireInterval);
		}

		// Randomize ghost cooldown if not already counting
		if (m_ghostCooldown < 0.f)
		{
			m_ghostCooldown = m_minGhostInterval +
				static_cast<float>(std::rand() % 100) / 100.f *
				(m_maxGhostInterval - m_minGhostInterval);
		}

		auto* animator = enemy.GetAnimator();
		if (animator)
		{
			animator->Play(enemy.GetLastHorizontalAnim());
			animator->Resume();
		}
	}

	std::unique_ptr<EnemyState> EnemyNormalState::Update(EnemyComponent& enemy, float deltaTime)
	{
		auto* movement = enemy.GetMovement();
		auto* animator = enemy.GetAnimator();
		if (!movement) return nullptr;

		// Pick direction at every cell
		if (!movement->IsMoving())
		{
			glm::ivec2 dir = ChooseDirection(enemy);
			movement->SetDesiredDirection(dir);

			if (animator)
			{
				if (dir.x > 0)      { enemy.SetLastHorizontalAnim("walk_right"); animator->Play("walk_right"); }
				else if (dir.x < 0) { enemy.SetLastHorizontalAnim("walk_left");  animator->Play("walk_left"); }
				else if (dir.y < 0) animator->Play("walk_up");
				else if (dir.y > 0) animator->Play("walk_down");
				animator->Resume();
			}
		}

		// Ghost transition
		m_ghostCooldown -= deltaTime;
		if (m_ghostCooldown <= 0.f && ShouldBecomeGhost(enemy))
			return std::make_unique<EnemyGhostState>();

		// Fire transition (Fygar only)
		if (enemy.GetEnemyType() == EnemyType::Fygar)
		{
			m_fireCooldown -= deltaTime;
			if (m_fireCooldown <= 0.f)
				return std::make_unique<EnemyFireBreathingState>(m_ghostCooldown);
		}

		return nullptr;
	}

	void EnemyNormalState::Exit(EnemyComponent& /*enemy*/)
	{
	}

	glm::ivec2 EnemyNormalState::ChooseDirection(EnemyComponent& enemy) const
	{
		auto* movement = enemy.GetMovement();
		auto* grid = enemy.GetGrid();
		auto* target = enemy.GetTarget();

		if (!target || !movement || !grid) return { 1, 0 };

		auto* targetMovement = target->GetComponent<GridMovementComponent>();
		if (!targetMovement) return { 1, 0 };

		const auto& myPos = movement->GetGridPosition();
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

		glm::ivec2 currentDir = movement->GetCurrentDirection();

		for (auto& opt : options)
		{
			glm::ivec2 next = myPos + opt.dir;
			opt.passable = grid->IsInBounds(next.x, next.y) &&
				grid->GetCellType(next.x, next.y) != CellType::Dirt;

			glm::ivec2 newDiff = targetPos - next;
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

	bool EnemyNormalState::ShouldBecomeGhost(EnemyComponent& enemy) const
	{
		auto* movement = enemy.GetMovement();
		auto* target = enemy.GetTarget();
		if (!movement || !target) return false;

		auto* targetMovement = target->GetComponent<GridMovementComponent>();
		if (!targetMovement) return false;

		const auto& myPos = movement->GetGridPosition();
		const auto& targetPos = targetMovement->GetGridPosition();

		int dist = std::abs(targetPos.x - myPos.x) + std::abs(targetPos.y - myPos.y);

		return dist > 5 || (std::rand() % 100 < 20);
	}

	// GhostState

	void EnemyGhostState::Enter(EnemyComponent& enemy)
	{
		m_hasBeenInDirt = false;

		auto* movement = enemy.GetMovement();
		if (movement) movement->SetGhostMode(true);

		auto* animator = enemy.GetAnimator();
		if (animator) animator->Play("ghost");
	}

	std::unique_ptr<EnemyState> EnemyGhostState::Update(EnemyComponent& enemy, float /*deltaTime*/)
	{
		auto* movement = enemy.GetMovement();
		if (!movement) return nullptr;

		// Only act when stopped at a cell
		if (movement->IsMoving()) return nullptr;

		// Check current cell for exit conditions
		auto* grid = enemy.GetGrid();
		const auto& pos = movement->GetGridPosition();
		if (grid)
		{
			CellType cell = grid->GetCellType(pos.x, pos.y);
			if (cell == CellType::Dirt)
				m_hasBeenInDirt = true;

			// Exit ghost at a Tunnel cell after passing through dirt
			if (m_hasBeenInDirt && cell == CellType::Tunnel)
				return std::make_unique<EnemyNormalState>();
		}

		// Chase the player
		glm::ivec2 dir = ChooseGhostDirection(enemy);
		movement->SetDesiredDirection(dir);

		return nullptr;
	}

	void EnemyGhostState::Exit(EnemyComponent& enemy)
	{
		auto* movement = enemy.GetMovement();
		if (movement) movement->SetGhostMode(false);
	}

	glm::ivec2 EnemyGhostState::ChooseGhostDirection(EnemyComponent& enemy) const
	{
		auto* movement = enemy.GetMovement();
		auto* target = enemy.GetTarget();
		if (!target || !movement) return { 1, 0 };

		auto* targetMovement = target->GetComponent<GridMovementComponent>();
		if (!targetMovement) return { 1, 0 };

		const auto& myPos = movement->GetGridPosition();
		const auto& targetPos = targetMovement->GetGridPosition();
		glm::ivec2 diff = targetPos - myPos;

		// Prefer the axis with greater distance
		if (std::abs(diff.x) >= std::abs(diff.y))
		{
			if (diff.x != 0)
				return { (diff.x > 0) ? 1 : -1, 0 };
			if (diff.y != 0)
				return { 0, (diff.y > 0) ? 1 : -1 };
		}
		else
		{
			if (diff.y != 0)
				return { 0, (diff.y > 0) ? 1 : -1 };
			if (diff.x != 0)
				return { (diff.x > 0) ? 1 : -1, 0 };
		}

		return { 1, 0 };
	}

	// InflatingState

	EnemyInflatingState::EnemyInflatingState(const glm::ivec2& attackDir)
		: m_attackDir(attackDir)
	{
	}

	void EnemyInflatingState::Enter(EnemyComponent& enemy)
	{
		m_inflateStage = 1;
		m_deflateTimer = 0.f;

		auto* movement = enemy.GetMovement();
		if (movement)
		{
			movement->SnapToCurrentCell();
			// Lock movement while inflating — this also ignores a player-
			// controlled Fygar's input so it can't wriggle off the pump.
			movement->SetFrozen(true);
		}

		// Sprites face left by default — flip when attacked from the right
		auto* render = enemy.GetOwner()->GetComponent<RenderComponent>();
		if (render)
			render->SetFlipHorizontal(m_attackDir.x > 0);

		auto* animator = enemy.GetAnimator();
		if (animator) animator->Play("inflate_1");
	}

	std::unique_ptr<EnemyState> EnemyInflatingState::Update(EnemyComponent& enemy, float deltaTime)
	{
		m_deflateTimer += deltaTime;
		if (m_deflateTimer >= enemy.Tuning().deflateTime)
		{
			m_deflateTimer = 0.f;
			--m_inflateStage;

			if (m_inflateStage <= 0)
			{
				if (enemy.IsPlayerControlled())
					return std::make_unique<EnemyIdleState>();
				else
					return std::make_unique<EnemyNormalState>();
			}

			auto* animator = enemy.GetAnimator();
			if (animator)
			{
				std::string animName = "inflate_" + std::to_string(m_inflateStage);
				animator->Play(animName);
			}
		}

		return nullptr;
	}

	void EnemyInflatingState::Exit(EnemyComponent& enemy)
	{
		auto* render = enemy.GetOwner()->GetComponent<RenderComponent>();
		if (render) render->SetFlipHorizontal(false);

		// Restore movement/input control (deflated back to normal or idle).
		auto* movement = enemy.GetMovement();
		if (movement) movement->SetFrozen(false);
	}

	std::unique_ptr<EnemyState> EnemyInflatingState::PumpOnce(EnemyComponent& enemy)
	{
		m_deflateTimer = 0.f;
		++m_inflateStage;

		if (m_inflateStage >= EnemyTuning::MaxInflateStages)
			return std::make_unique<EnemyPoppedState>();

		auto* animator = enemy.GetAnimator();
		if (animator)
		{
			std::string animName = "inflate_" + std::to_string(m_inflateStage);
			animator->Play(animName);
		}

		return nullptr;
	}

	// PoppedState

	void EnemyPoppedState::Enter(EnemyComponent& enemy)
	{
		ServiceLocator::GetSoundService().PlaySound(Sounds::MonsterBlow);
		enemy.Notify(EVENT_ENEMY_KILLED, enemy.GetOwner());
		enemy.GetOwner()->MarkForDestroy();
	}

	std::unique_ptr<EnemyState> EnemyPoppedState::Update(EnemyComponent& /*enemy*/, float /*deltaTime*/)
	{
		return nullptr;
	}

	void EnemyPoppedState::Exit(EnemyComponent& /*enemy*/)
	{
	}

	// CrushedState

	void EnemyCrushedState::Enter(EnemyComponent& enemy)
	{
		ServiceLocator::GetSoundService().PlaySound(Sounds::MonsterBlow);
		enemy.Notify(EVENT_ENEMY_KILLED, enemy.GetOwner());
		enemy.GetOwner()->MarkForDestroy();
	}

	std::unique_ptr<EnemyState> EnemyCrushedState::Update(EnemyComponent& /*enemy*/, float /*deltaTime*/)
	{
		return nullptr;
	}

	void EnemyCrushedState::Exit(EnemyComponent& /*enemy*/)
	{
	}

	// FireBreathingState

	EnemyFireBreathingState::EnemyFireBreathingState(float ghostCooldown)
		: m_ghostCooldown(ghostCooldown)
	{
	}

	void EnemyFireBreathingState::Enter(EnemyComponent& enemy)
	{
		m_fireTimer = 0.f;
		m_extendTimer = 0.f;
		m_currentRange = 0;

		auto* movement = enemy.GetMovement();
		if (movement) movement->SnapToCurrentCell();

		// Fire goes in the direction the Fygar is facing
		const std::string& lastAnim = enemy.GetLastHorizontalAnim();
		m_fireDirection = (lastAnim == "walk_left") ? glm::ivec2{ -1, 0 } : glm::ivec2{ 1, 0 };

		auto* animator = enemy.GetAnimator();
		if (animator) animator->Play(lastAnim);
	}

	std::unique_ptr<EnemyState> EnemyFireBreathingState::Update(EnemyComponent& enemy, float deltaTime)
	{
		m_fireTimer += deltaTime;
		m_ghostCooldown -= deltaTime;

		// Extend fire range over time
		m_extendTimer += deltaTime;
		if (m_extendTimer >= m_extendInterval && m_currentRange < m_maxFireRange)
		{
			m_extendTimer = 0.f;
			++m_currentRange;
		}

		// Done breathing fire
		if (m_fireTimer >= m_fireDuration)
		{
			if (enemy.IsPlayerControlled())
				return std::make_unique<EnemyIdleState>();
			else
				return std::make_unique<EnemyNormalState>(m_ghostCooldown);
		}

		return nullptr;
	}

	void EnemyFireBreathingState::Exit(EnemyComponent& /*enemy*/)
	{
	}
}
