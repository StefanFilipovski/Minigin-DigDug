#include "EnemyStates.h"
#include "EnemyComponent.h"
#include "GridMovementComponent.h"
#include "SpriteAnimatorComponent.h"
#include "RenderComponent.h"
#include "EventIds.h"
#include "GameObject.h"
#include <cstdlib>
#include <string>

namespace dae
{
	// NormalState
	void EnemyNormalState::Enter(EnemyComponent& enemy)
	{
		// Randomize ghost cooldown each time we enter Normal
		m_ghostCooldown = enemy.GetMinGhostInterval() +
			static_cast<float>(std::rand() % 100) / 100.f *
			(enemy.GetMaxGhostInterval() - enemy.GetMinGhostInterval());

		// Randomize fire cooldown for Fygar
		if (enemy.GetEnemyType() == EnemyType::Fygar)
		{
			m_fireCooldown = enemy.GetMinFireInterval() +
				static_cast<float>(std::rand() % 100) / 100.f *
				(enemy.GetMaxFireInterval() - enemy.GetMinFireInterval());
		}

		m_dirChangeTimer = 0.f;

		auto* animator = enemy.GetAnimator();
		if (animator)
		{
			animator->Play(enemy.GetLastHorizontalAnim());
			animator->Resume();
		}
	}

	void EnemyNormalState::Update(EnemyComponent& enemy, float deltaTime)
	{
		auto* movement = enemy.GetMovement();
		auto* animator = enemy.GetAnimator();
		if (!movement) return;

		m_dirChangeTimer += deltaTime;
		if (m_dirChangeTimer >= enemy.GetDirChangeInterval())
		{
			m_dirChangeTimer = 0.f;

			if (!movement->IsMoving())
			{
				glm::ivec2 dir = enemy.ChooseDirection();
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
		}

		if (movement->IsMoving())
		{
			movement->SetDesiredDirection(movement->GetCurrentDirection());
		}

		// Ghost transition
		m_ghostCooldown -= deltaTime;
		if (m_ghostCooldown <= 0.f && enemy.ShouldBecomeGhost())
		{
			enemy.ChangeState(std::make_unique<EnemyGhostState>());
			return;
		}

		// Fire transition (Fygar only)
		if (enemy.GetEnemyType() == EnemyType::Fygar)
		{
			m_fireCooldown -= deltaTime;
			if (m_fireCooldown <= 0.f)
			{
				enemy.ChangeState(std::make_unique<EnemyFireBreathingState>());
				return;
			}
		}
	}

	void EnemyNormalState::Exit(EnemyComponent& /*enemy*/)
	{
	}

	// GhostState

	void EnemyGhostState::Enter(EnemyComponent& enemy)
	{
		m_ghostTimer = 0.f;
		m_dirChangeTimer = 0.f;

		auto* movement = enemy.GetMovement();
		if (movement) movement->SetGhostMode(true);

		auto* animator = enemy.GetAnimator();
		if (animator) animator->Play("ghost");
	}

	void EnemyGhostState::Update(EnemyComponent& enemy, float deltaTime)
	{
		auto* movement = enemy.GetMovement();
		if (!movement) return;

		m_ghostTimer += deltaTime;

		// Move toward the player while in ghost form
		m_dirChangeTimer += deltaTime;
		if (m_dirChangeTimer >= enemy.GetDirChangeInterval())
		{
			m_dirChangeTimer = 0.f;

			if (!movement->IsMoving())
			{
				glm::ivec2 dir = enemy.ChooseGhostDirection();
				movement->SetDesiredDirection(dir);
			}
		}

		if (movement->IsMoving())
		{
			movement->SetDesiredDirection(movement->GetCurrentDirection());
		}

		// Only exit ghost mode after duration expires and on a tunnel/surface cell
		if (m_ghostTimer >= enemy.GetGhostDuration() && enemy.IsInTunnel())
		{
			enemy.ChangeState(std::make_unique<EnemyNormalState>());
			return;
		}
	}

	void EnemyGhostState::Exit(EnemyComponent& enemy)
	{
		auto* movement = enemy.GetMovement();
		if (movement) movement->SetGhostMode(false);
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
		if (movement) movement->SetDesiredDirection({ 0, 0 });

		// Sprites face left by default — flip when attacked from the right
		auto* render = enemy.GetOwner()->GetComponent<RenderComponent>();
		if (render)
			render->SetFlipHorizontal(m_attackDir.x > 0);

		auto* animator = enemy.GetAnimator();
		if (animator) animator->Play("inflate_1");
	}

	void EnemyInflatingState::Update(EnemyComponent& enemy, float deltaTime)
	{
		m_deflateTimer += deltaTime;
		if (m_deflateTimer >= enemy.GetDeflateTime())
		{
			m_deflateTimer = 0.f;
			--m_inflateStage;

			if (m_inflateStage <= 0)
			{
				// Fully deflated — return to Normal
				enemy.ChangeState(std::make_unique<EnemyNormalState>());
				return;
			}

			auto* animator = enemy.GetAnimator();
			if (animator)
			{
				std::string animName = "inflate_" + std::to_string(m_inflateStage);
				animator->Play(animName);
			}
		}
	}

	void EnemyInflatingState::Exit(EnemyComponent& enemy)
	{
		// Reset flip when leaving inflate
		auto* render = enemy.GetOwner()->GetComponent<RenderComponent>();
		if (render) render->SetFlipHorizontal(false);
	}

	void EnemyInflatingState::PumpOnce(EnemyComponent& enemy)
	{
		m_deflateTimer = 0.f;
		++m_inflateStage;

		if (m_inflateStage >= enemy.GetMaxInflateStages())
		{
			// Pop
			enemy.ChangeState(std::make_unique<EnemyPoppedState>());
			return;
		}

		auto* animator = enemy.GetAnimator();
		if (animator)
		{
			std::string animName = "inflate_" + std::to_string(m_inflateStage);
			animator->Play(animName);
		}
	}

	//PoppedStat
	void EnemyPoppedState::Enter(EnemyComponent& enemy)
	{
		enemy.Notify(EVENT_ENEMY_KILLED, enemy.GetOwner());
		enemy.GetOwner()->MarkForDestroy();
	}

	void EnemyPoppedState::Update(EnemyComponent& /*enemy*/, float /*deltaTime*/)
	{
	}

	void EnemyPoppedState::Exit(EnemyComponent& /*enemy*/)
	{
	}

	// CrushedState
	void EnemyCrushedState::Enter(EnemyComponent& enemy)
	{
		enemy.Notify(EVENT_ENEMY_KILLED, enemy.GetOwner());
		enemy.GetOwner()->MarkForDestroy();
	}

	void EnemyCrushedState::Update(EnemyComponent& /*enemy*/, float /*deltaTime*/)
	{
	}

	void EnemyCrushedState::Exit(EnemyComponent& /*enemy*/)
	{
	}

	// FireBreathingState (Fygar only)

	void EnemyFireBreathingState::Enter(EnemyComponent& enemy)
	{
		m_fireTimer = 0.f;
		m_extendTimer = 0.f;
		m_currentRange = 0;

		auto* movement = enemy.GetMovement();
		if (movement) movement->SetDesiredDirection({ 0, 0 });

		// Fire goes in the direction the Fygar is facing (horizontal only)
		const std::string& lastAnim = enemy.GetLastHorizontalAnim();
		m_fireDirection = (lastAnim == "walk_left") ? glm::ivec2{ -1, 0 } : glm::ivec2{ 1, 0 };

		auto* animator = enemy.GetAnimator();
		if (animator) animator->Play(lastAnim);
	}

	void EnemyFireBreathingState::Update(EnemyComponent& enemy, float deltaTime)
	{
		m_fireTimer += deltaTime;

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
			enemy.ChangeState(std::make_unique<EnemyNormalState>());
			return;
		}
	}

	void EnemyFireBreathingState::Exit(EnemyComponent& /*enemy*/)
	{
	}
}
