#include "PlayerAnimController.h"
#include "GridMovementComponent.h"
#include "SpriteAnimatorComponent.h"
#include "PlayerCollisionComponent.h"
#include "PumpComponent.h"
#include "GameObject.h"

namespace dae
{
	void PlayerAnimController::Update(float /*deltaTime*/)
	{
		if (!m_cached)
		{
			m_pMovement = GetOwner()->GetComponent<GridMovementComponent>();
			m_pAnimator = GetOwner()->GetComponent<SpriteAnimatorComponent>();
			m_pCollision = GetOwner()->GetComponent<PlayerCollisionComponent>();
			m_pPump = GetOwner()->GetComponent<PumpComponent>();
			m_cached = true;
		}

		if (!m_pMovement || !m_pAnimator) return;

		if (m_pCollision && m_pCollision->IsDead()) return;

		if (m_pPump && m_pPump->IsFiring()) return;

		const auto& dir = m_pMovement->GetCurrentDirection();
		bool moving = m_pMovement->IsMoving();

		if (!moving)
		{
			m_pAnimator->Pause();
			return;
		}

		m_pAnimator->Resume();

		// Static names avoid building a new string every frame the player moves
		static const std::string walkAnims[]{ "walk_right", "walk_left", "walk_up", "walk_down" };
		static const std::string digAnims[]{ "dig_right", "dig_left", "dig_up", "dig_down" };
		const auto& anims = m_pMovement->IsDigging() ? digAnims : walkAnims;

		if (dir.x > 0)
			m_pAnimator->Play(anims[0]);
		else if (dir.x < 0)
			m_pAnimator->Play(anims[1]);
		else if (dir.y < 0)
			m_pAnimator->Play(anims[2]);
		else if (dir.y > 0)
			m_pAnimator->Play(anims[3]);
	}
}