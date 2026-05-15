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
			m_cached = true;
		}

		if (!m_pMovement || !m_pAnimator) return;

		// Don't override death animation — let it play to completion
		if (m_pCollision && m_pCollision->IsDead()) return;

		// Don't override pump animation — PumpComponent manages it
		auto* pump = GetOwner()->GetComponent<PumpComponent>();
		if (pump && pump->IsFiring()) return;

		const auto& dir = m_pMovement->GetCurrentDirection();
		bool moving = m_pMovement->IsMoving();

		if (!moving)
		{
			m_pAnimator->Pause();
			return;
		}

		m_pAnimator->Resume();

		std::string prefix = m_pMovement->IsDigging() ? "dig" : "walk";

		if (dir.x > 0)
			m_pAnimator->Play(prefix + "_right");
		else if (dir.x < 0)
			m_pAnimator->Play(prefix + "_left");
		else if (dir.y < 0)
			m_pAnimator->Play(prefix + "_up");
		else if (dir.y > 0)
			m_pAnimator->Play(prefix + "_down");
	}
}