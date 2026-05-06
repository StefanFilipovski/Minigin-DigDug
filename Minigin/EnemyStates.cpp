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

		// After max duration, wait until tunnel
		if (m_ghostTimer >= enemy.GetGhostDuration())
		{
			if (enemy.IsInTunnel())
			{
				enemy.ChangeState(std::make_unique<EnemyNormalState>());
				return;
			}
		}

		// Small delay avoids exiting immediately after entering
		if (enemy.IsInTunnel() && m_ghostTimer > 1.f)
		{
			enemy.ChangeState(std::make_unique<EnemyNormalState>());
			return;
		}

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
}
