#include "PlayerAnimController.h"
#include "GridMovementComponent.h"
#include "SpriteAnimatorComponent.h"
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
			m_cached = true;
		}

		if (!m_pMovement || !m_pAnimator) return;

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

		// Use dig animation when moving into dirt, walk when in tunnels
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